
connectorgame: mcp.h leds.h audio.h

connectorgame: connectorgame.c mcp.c leds.c audio.c
	gcc -Wno-unused -W -Wall -o $@ $^ -lwiringPi -lws2811 -lasound

test: connectorgame
	sudo ./connectorgame
