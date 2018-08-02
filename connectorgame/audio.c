#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "main.h"
#include "audio.h"

#define PI 3.14159265

static snd_pcm_t *pcm_handle = NULL;

static unsigned char *mmap_pos = NULL;
static size_t mmap_len = 0;
static size_t play_pos = 0;

static char *wav_audiofiles[] = WAV_AUDIOFILES;

static struct {
    int16_t *samples;
    long length;
    long position;
    int repeat;
    struct synth_s {
        double d1;
        double d2;
        double c;
        double fcur;
        double vcur;
        double fto;
        double vto;
        int steps;
    } synth[SYNTH_CHANNELS];
} pcm_channels[WAV_CHANNELS];

static struct {
    int16_t *samples;
    long length;
    int repeat;
} wavfiles[WAV_COUNT];

static size_t read_le32(unsigned char *bytes)
{
    return bytes[0] + (bytes[1]<<8) + (bytes[2]<<16) + (bytes[3]<<24);
}

static int read_le16(unsigned char *bytes)
{
    return bytes[0] + (bytes[1]<<8);
}

static int read_wavfile(char *name, int wc)
{
    int fd;
    int err;
    char pathname[strlen(PCM_PATH)+strlen(name)+1];
    strcpy(pathname, PCM_PATH);
    strcat(pathname, name);
    fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open audio %s: %s\n", pathname, strerror(fd));
        return fd;
    }
    struct stat statbuf;
    if ((err = fstat(fd, &statbuf)) < 0) {
        close(fd);
        fprintf(stderr, "Failed to stat audio %s: %s\n", pathname, strerror(err));
        return err;
    }
    size_t wav_len = statbuf.st_size;
    unsigned char *wavdata = mmap(NULL, wav_len, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (wavdata == NULL) {
        fprintf(stderr, "Failed to mmap file %s: %s\n", pathname, strerror(errno));
        return -1;
    }
    if ((wav_len < 44) || memcmp(wavdata+0, "RIFF", 4) || memcmp(wavdata+8, "WAVEfmt ", 8)) {
        fprintf(stderr, "Wav file format error on %s\n", pathname);
        return -1;
    }
    size_t fmtlen = read_le32(wavdata+16);
    if ((wav_len < fmtlen+28) || memcmp(wavdata+20+fmtlen, "data", 4)) {
        fprintf(stderr, "Wav file format error on %s\n", pathname);
        return -1;
    }
    if (read_le16(wavdata+20) != 1) {
        fprintf(stderr, "Wav file not PCM %s\n", pathname);
        return -1;
    }
    if (read_le16(wavdata+22) != 2) {
        fprintf(stderr, "Wav file not stereo: %s\n", pathname);
        return -1;
    }
    if (read_le32(wavdata+24) != PCM_RATE) {
        fprintf(stderr, "Wav file not %dHz: %s\n", PCM_RATE, pathname);
        return -1;
    }
    size_t wavdatalen = read_le32(wavdata+fmtlen+24);
    if ((wav_len < wavdatalen+fmtlen+28)) {
        fprintf(stderr, "Wav file size error on %s\n", pathname);
        return -1;
    }
    wavfiles[wc].samples = (int16_t *)(wavdata+fmtlen+28);
    wavfiles[wc].length = wavdatalen/4;
    wavfiles[wc].repeat = 0;
    return 0;
}

static int read_wavfiles(void)
{
    int ret = 0;
    for (int wc = 0; wc < WAV_COUNT; wc++) {
        if (read_wavfile(wav_audiofiles[wc], wc) < 0) {
            ret = -1;
            wavfiles[wc].samples = NULL;
        }
    }
    return ret;
}

int init_audio(void)
{
    for (int c = 0; c < WAV_CHANNELS; c++) {
        pcm_channels[c].samples = NULL;
    }
    return read_wavfiles();
}

int fini_audio(void)
{
    if (pcm_handle) snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
    return 0;
}

static void pcm_mix_buffer(int16_t *buffer, long len)
{
    memset(buffer, 0, len*2*sizeof(*buffer));
    for (int c = 0; c < WAV_CHANNELS; c++) {
        if (pcm_channels[c].length == -1) {
            struct synth_s *synth = pcm_channels[c].synth;
            for (int sc = 0; sc < SYNTH_CHANNELS; sc++) {
                if (synth[sc].steps > 0) {
                    synth[sc].fcur += (synth[sc].fto - synth[sc].fcur) / synth[sc].steps;
                    synth[sc].vcur += (synth[sc].vto - synth[sc].vcur) / synth[sc].steps;
                    synth[sc].steps -= 1;
                    if (synth[sc].fcur > 0.0) {
                        double fstep = PI * 2 * synth[sc].fcur / PCM_RATE;
                        synth[sc].c  = cos(fstep) * 2;
                        double rvsin = synth[sc].d1;
                        if (rvsin < -1.0) rvsin = -1.0;
                        if (rvsin >  1.0) rvsin =  1.0;
                        if (synth[sc].d1 < synth[sc].d2) {
                            synth[sc].d2 = sin(asin(rvsin) + fstep);
                        } else {
                            synth[sc].d2 = sin(asin(rvsin) - fstep);
                        }
                    } else {
                        synth[sc].c = 0.0;
                        synth[sc].d1 = 0.0;
                        synth[sc].d2 = 0.0;
                    }
                }
            }
            for (int s = 0; s < len*2; s++) {
                int sc = s % 2;
                double val = synth[sc].d1 * synth[sc].vcur;
                while ((sc += 2) < SYNTH_CHANNELS) {
                    val += synth[sc].d1 * synth[sc].vcur;
                }
                long byteval = (long)(val * 0x7FFF);
                if (val < -0x7FFF) val = -0x7FFF;
                if (val >  0x7FFF) val =  0x7FFF;
                buffer[s] += (int16_t)(byteval / WAV_CHANNELS);
                for (sc = s % 2; sc < SYNTH_CHANNELS; sc += 2) {
                    double d0 = synth[sc].d1 * synth[sc].c - synth[sc].d2;
                    synth[sc].d2 = synth[sc].d1;
                    synth[sc].d1 = d0;
                }
            }
        } else {
            long pos = 0;
            while (pcm_channels[c].samples && (pos < len)) {
                long mixsize = len - pos;
                if (mixsize > (pcm_channels[c].length - pcm_channels[c].position)) {
                    mixsize = (pcm_channels[c].length - pcm_channels[c].position);
                }
                int16_t *samples = pcm_channels[c].samples+(pcm_channels[c].position*2);
                int16_t *bufp = buffer + pos*2;
                for (long s = 0; s < mixsize; s++) {
                    bufp[s*2  ] += (samples[s*2  ] / WAV_CHANNELS);
                    bufp[s*2+1] += (samples[s*2+1] / WAV_CHANNELS);
                }
                long newpos = pcm_channels[c].position + mixsize;
                if (newpos < pcm_channels[c].length) {
                    pcm_channels[c].position = newpos;
                } else {
                    if (pcm_channels[c].repeat) {
                        pcm_channels[c].position = 0;
                    } else {
                        pcm_channels[c].samples = NULL;
                    }
                }
                pos += mixsize;
            }
        }
    }
}

void audio_mainloop(void)
{
    int err;
    if (pcm_handle == NULL) {
        if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            fprintf(stderr, "Open PCM device %s failed: %s\n", PCM_DEVICE, snd_strerror(err));
            pcm_handle = NULL;
        } else if ((err = snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, PCM_CHANNELS, PCM_RATE, 1, PCM_LATENCY)) < 0) {
            fprintf(stderr, "Set PCM device %s failed: %s\n", PCM_DEVICE, snd_strerror(err));
            snd_pcm_close(pcm_handle);
            pcm_handle = NULL;
        }
    }
    if (pcm_handle) {
        long play_len = snd_pcm_avail(pcm_handle);
        if (play_len < 0) {
            fprintf(stderr, "snd_pcm_avail error: %s\n", snd_strerror(play_len));
            snd_pcm_close(pcm_handle);
            pcm_handle = NULL;
            return;
        }
        if (play_len == 0) {
            return;
        }
        int16_t buffer[play_len*2];
        pcm_mix_buffer(buffer, play_len);
        int written;
        written = snd_pcm_writei(pcm_handle, buffer, play_len);
        if (written < 0) {
            fprintf(stderr, "Write to %s failed: %s\n", PCM_DEVICE, snd_strerror(written));
            snd_pcm_close(pcm_handle);
            pcm_handle = NULL;
        } else if (written < play_len) {
            fprintf(stderr, "Write to %s failed: Short write(%d)\n", PCM_DEVICE, written);
        }
    }
}

int audio_play_file(int channel, enum wav_sounds sound)
{
    if (channel >= WAV_CHANNELS || sound >= WAV_COUNT) {
        fprintf(stderr, "audio_play_file argument error\n");
        return -1;
    }
    pcm_channels[channel].samples = wavfiles[sound].samples;
    pcm_channels[channel].length  = wavfiles[sound].length;
    pcm_channels[channel].repeat  = wavfiles[sound].repeat;
    pcm_channels[channel].position = 0;
    pdebug("audio_play_file(%d, %s)", channel, wav_audiofiles[sound]);
    return 0;
}

/* Sinusgolf genereren:
 * Gebaseerd op de vergelijking:  sin(x+y)+sin(x-y) = 2*cos(y)*sin(x)
 * Omgezet, waar p = 2PI * (frequency/samplerate), oftewel de afstand tussen twee samples:
 *   sin(x+p) = 2*cos(p)*sin(x) - sin(x-p)
 *  We willen dat d(x) = sin(p*x)
 *   d0 = sin(0) = 0, d1 = sin(p)
 *   c = 2*cos(p)
 *  Dus: d(x+1) = sin(p*x+p)) = 2*cos(p)*sin(p*x) - sin(p*x-p) = c * d(x) - d(x-1)
 * Met de twee vorige waardes kun je de huidige berekenen met 1 vermenigvuldiging en 1 optelling
 */
int audio_play_synth(int channel, int synthchannel, double frequency, double volume, int steps)
{
    if (channel >= WAV_CHANNELS || synthchannel >= SYNTH_CHANNELS) {
        fprintf(stderr, "audio_play_file argument error\n");
        return -1;
    }
    struct synth_s *synth = pcm_channels[channel].synth;
    if (pcm_channels[channel].length != -1) {
        /* First time init */
        for (int sc = 0; sc < SYNTH_CHANNELS; sc++) {
            synth[sc].d1 = 0;
            synth[sc].d2 = 0;
            synth[sc].c = 0;
            synth[sc].fcur = 0;
            synth[sc].vcur = 0;
            synth[sc].fto = 0;
            synth[sc].vto = 0;
            synth[sc].steps = 0;
        }
        pcm_channels[channel].length = -1;
        pcm_channels[channel].samples = NULL;
    }
    synth[synthchannel].fto = frequency;
    synth[synthchannel].vto = volume;
    synth[synthchannel].steps = steps;
    pdebug("audio_play_synth(%d, %d, %f, %f, %d)", channel, synthchannel, frequency, volume, steps);
    return 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
