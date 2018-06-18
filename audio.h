/* Definities voor audio aansturing */

#define PCM_DEVICE      "default"
#define PCM_LATENCY     200000
#define PCM_CHANNELS    2
#define PCM_RATE        44100

#define PCM_BUFSIZE     (4410*2*2)

#define PCM_PATH        "/home/pi/audio/"

int init_audio(void);
int fini_audio(void);
void audio_mainloop(void);
int audio_play_file(char *path);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
