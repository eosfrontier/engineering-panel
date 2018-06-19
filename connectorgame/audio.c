#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "audio.h"

static snd_pcm_t *pcm_handle = NULL;
static size_t play_pos = 0;

static unsigned char *mmap_pos = NULL;
static size_t mmap_len = 0;

int init_audio(void)
{
    return 0;
}

int fini_audio(void)
{
    if (pcm_handle) snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
    return 0;
}

void audio_mainloop(void)
{
    int err;
    if (mmap_pos && (play_pos < mmap_len)) {
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
            unsigned int play_len = snd_pcm_avail(pcm_handle);
            int written;
            if (play_len > (mmap_len - play_pos)) play_len = mmap_len - play_pos;
            written = snd_pcm_writei(pcm_handle, mmap_pos + play_pos, play_len);
            if (written <= 0) {
                fprintf(stderr, "Write to %s failed: %s\n", PCM_DEVICE, snd_strerror(written));
                snd_pcm_close(pcm_handle);
                pcm_handle = NULL;
            } else {
                play_pos += written * 4;
                if (play_pos >= mmap_len) {
                    munmap(mmap_pos, mmap_len);
                    mmap_pos = NULL;
                }
            }
        }
    }
}

int audio_play_file(char *name)
{
    int fd;
    int err;
    char pathname[strlen(PCM_PATH)+strlen(name)+1];
    if (mmap_pos) {
        munmap(mmap_pos, mmap_len);
        mmap_pos = NULL;
    }
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
    mmap_len = statbuf.st_size;
    mmap_pos = mmap(NULL, mmap_len, PROT_READ, MAP_SHARED, fd, 0);
    play_pos = 0;
    close(fd);
    if (mmap_pos == NULL) {
        mmap_len = 0;
        fprintf(stderr, "Failed to mmap file %s: %s\n", pathname, strerror(errno));
        return -1;
    }
    play_pos = 44;
    // printf("Playing %s\n", pathname);
    return 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
