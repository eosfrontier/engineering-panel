
connectorgame: mcp.h leds.h audio.h

connectorgame: connectorgame.c mcp.c leds.c audio.c
	gcc -O2 -Wno-unused -W -Wall -o $@ $^ -lm -lwiringPi -lws2811 -lasound

test: connectorgame
	sudo ./connectorgame
