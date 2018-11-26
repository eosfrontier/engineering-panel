/* Definities voor audio aansturing */

#define PCM_DEVICE      "default"
#define PCM_LATENCY     200000
#define PCM_CHANNELS    2
#define PCM_RATE        44100

#define PCM_PATH        "/home/pi/shuttle-panels/audio/"

enum wav_sounds {
    WAV_ON,
    WAV_OFF,
    WAV_READY,
    WAV_BOOTING,
    WAV_SPARK1,
    WAV_SPARK2,
    WAV_SPARK3,
    WAV_SPARK4,
    WAV_SPARK_SHORT,
    WAV_ENGINE_OFF,
    WAV_ENGINE_ON,
    WAV_COUNT
};

enum synth_waveforms {
    SYNTH_NONE,
    SYNTH_SINE,
    SYNTH_TRIANGLE,
    SYNTH_SAWTOOTH
};

#define WAV_AUDIOFILES {"on.wav","off.wav","ready.wav","booting.wav","spark1.wav","spark2.wav","spark3.wav","spark4.wav","spark-short.wav","engineready.wav","engineoff.wav"}
#define WAV_CHANNELS 2
#define SYNTH_CHANNELS 14

int init_audio(void);
int fini_audio(void);
void audio_mainloop(void);
int audio_play_file(int channel, enum wav_sounds sound);
int audio_synth_wave(int channel, int synthchannel, int waveform);
int audio_synth_freq_vol(int channel, int synthchannel, double frequency, double volume, int steps);
int audio_synth_modulate(int channel, int synthchannel, int modchannel);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
