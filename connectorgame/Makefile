HEADERS = main.h mcp.h leds.h audio.h game.h

%.o: %.c $(HEADERS)
	gcc -O2 -Wno-unused -W -Wall -o $@ -c $<

connectorgame: main.o mcp.o leds.o audio.o game.o
	gcc -O2 -Wno-unused -W -Wall -o $@ $^ -lm -lwiringPi -lws2811 -lasound

test: connectorgame
	sudo systemctl stop connectorgame.service
	sudo ./connectorgame debug

run: connectorgame
	sudo systemctl stop connectorgame.service
	sudo ./connectorgame
