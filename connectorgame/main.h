#define SCANRATE  2
#define FRAMERATE 50
#define SLEEPTIME (1000000/FRAMERATE)

extern int debugging;
void pdebug(const char *format, ...);
