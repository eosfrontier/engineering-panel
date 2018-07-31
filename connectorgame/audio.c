#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "main.h"
#include "audio.h"

static snd_pcm_t *pcm_handle = NULL;

static unsigned char *mmap_pos = NULL;
static size_t mmap_len = 0;
static size_t play_pos = 0;

static char *wav_audiofiles[] = WAV_AUDIOFILES;

static struct {
    int16_t *samples;
    size_t length;
    size_t position;
    int repeat;
} pcm_channels[PCM_CHANNELS];

static struct {
    int16_t *samples;
    size_t length;
} wavfiles[WAV_COUNT];

static size_t read_le32(char *bytes)
{
    return bytes[0] + bytes[1]<<8 + bytes[2]<<16 + bytes[3]<<24;
}

static int read_le16(char *bytes)
{
    return bytes[0] + bytes[1]<<8;
}

static int read_wavfile(char *name, int wc)
{
    int fd;
    int err;
    char pathname[strlen(PCM_PAT)+strlen(name)+1];
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
        fprintf(stderr, "Wav file not stereo %s\n", pathname);
        return -1;
    }
    if (read_le32(wavdata+24) != 44100) {
        fprintf(stderr, "Wav file not 44100Hz %s\n", pathname);
        return -1;
    }
    size_t wavdatalen = read_le32(wavdata+fmtlen+24);
    if ((wav_len < wavdatalen+fmtlen+28)) {
        fprintf(stderr, "Wav file size error on %s\n", pathname);
        return -1;
    }
    wavfiles[wc].samples = (int16_t *)(wavdata+fmtlen+28);
    wavfiles[wc].length = wavdatalen/4;
    return 0;
}

static int read_wavfiles(void)
{
    int ret = 0;
    for (int wc = 0; wc < WAV_COUNT; wc++) {
        if (read_wavfile(wavfiles[wc], wc) < 0) ret = -1;
    }
    return ret;
}

int init_audio(void)
{
    for (int c = 0; c < PCM_CHANNELS; c++) {
        pcm_channels[c].samples = NULL;
    }
    return read_wavfiles(PCM_PATH);
}

int fini_audio(void)
{
    if (pcm_handle) snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
    return 0;
}

static void pcm_mix_buffer(int16_t *buffer, long len)
{
    while (len > 0) {
        /* Get smallest size to mix */
        long mixsize = len;
        for (int c = 0; c < PCM_CHANNELS; c++) {
            if (pcm_channels[c].samples) {
                if (mixsize > (pcm_channels[c].length - pcm_channels[c].position)) {
                    mixsize = (pcm_channels[c].length - pcm_channels[c].position);
                }
            }
        }
        memset(buffer, 0, mixsize*2*sizeof(*buffer));
        for (int c = 0; c < PCM_CHANNELS; c++) {
            if (pcm_channels[c].samples) {
                for (long s = 0; s < mixsize*2; s++) {
                    buffer[s] += (pcm_channels[c].samples[position+s] / PCM_CHANNELS);
                }
                size_t newpos = pcm_channels[c].position + mixsize;
                if (newpos < pcm_channels[c].length) {
                    pcm_channels[c].position = newpos;
                } else {
                    if (pcm_channels[c].repeat) {
                        pcm_channels[c].position = 0;
                    } else {
                        pcm_channels[c].samples = NULL;
                    }
                }
            }
        }
        len -= mixsize;
    }
}

void audio_mainloop(void)
{
    unsigned int 
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
        int16_t buffer[play_len*2];
        pcm_mix_buffer(buffer, play_len);
        int written;
        written = snd_pcm_writei(pcm_handle, buffer, play_len);
        if (written <= 0) {
            fprintf(stderr, "Write to %s failed: %s\n", PCM_DEVICE, snd_strerror(written));
            snd_pcm_close(pcm_handle);
            pcm_handle = NULL;
        }
    }
}

int audio_play_file(int channel, enum wav_sounds sound, int repeat)
{
    if (channel >= PCM_CHANNELS || sound >= WAV_COUNT) {
        fprintf(stderr, "audio_play_file argument error\n");
        return -1;
    }
    pcm_channels[channel].samples = wavfiles[sound].samples;
    pcm_channels[channel].length = wavfiles[sound].length;
    pcm_channels[channel].repeat = repeat;
    pcm_channels[channel].position = 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
