HEADERS = main.h mcp.h leds.h audio.h game.h comm.h

%.o: %.c $(HEADERS)
	gcc -O2 -Wno-unused -W -Wall -o $@ -c $<

connectorgame: main.o mcp.o leds.o audio.o game.o comm.o
	gcc -O2 -Wno-unused -W -Wall -o $@ $^ -lm -lwiringPi -lws2811 -lasound

test: connectorgame
	sudo systemctl stop connectorgame.service
	sudo ./connectorgame debug

run: connectorgame
	sudo systemctl stop connectorgame.service
	sudo ./connectorgame

restart: connectorgame
	sudo systemctl stop connectorgame.service
	sudo systemctl start connectorgame.service

install: connectorgame
	sudo cp connectorgame.service /etc/systemd/system/connectorgame.service
	sudo systemctl enable connectorgame.service
	sudo systemctl start connectorgame.service
