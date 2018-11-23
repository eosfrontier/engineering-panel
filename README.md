# Shuttle Panels

Collection of code to run 'shuttle panels' - physical panels that represent parts of a shuttle (used in a LARP), which contain puzzles to physrep repairing these shuttles.

## The panels

NB: Currently only the connector panel exists

### Connector panel

35x50 panel with 100 banana plug sockets and 96 leds.

Uses a raspberry pi zero (or zeroW), seven MCP23017 chips, WS2812B leds (neopixels), and an i2s chip plus amplifier.

The MCP23017s interface with bananaplug connectors which the software reads to see which sockets are joined by a cable.  This then runs a mastermind-like puzzle where the LEDs give clues to the game.

### Access panel

NB: To be designed, subject to change.

Small panel to provide a puzzle for opening sci-fi doors.

## Installation instructions

### Install raspbian-lite on an SD-card

See https://www.raspberrypi.org/documentation/installation/installing-images/README.md

### Mount the boot partition and add ssh and wpa files

* Add a file named 'ssh' in the root (content does not matter)
* Add a file named wpa_supplicant.conf in the root with the following content (insert appropriate WLAN ssid and passwords)
```
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=NL

network={
        ssid="<home wifi ssd>"
        psk="<home wifi password>"
        id_str="thuis"
}

network={
        ssid="<eos wifi ssd>"
        psk="<eos wifi password>"
        id_str="eos"
}
```

### Boot the SD card on the pi and run the install script

* Insert the SD card on the Pi (ZeroW) mounted to the panel and boot it.
* Find the new IP address and SSH into the PI
* Immediately change the password from the default
* Download the install-connectorgame.sh: wget https://github.com/willemm/shuttle-panels/raw/master/install/install-connectorgame.sh
* Make it executable: chmod +x install-connectorgame.sh
* Run the script: ./install-connectorgame.sh
* Add SL password: echo -n <spelleider-wachtwoord> > /home/pi/shuttle-panels/spelleiderwachtwoord.txt

### Make the SD card readonly with OverlayFS

* cd into /home/pi/shuttle-panels/ro-overlayfs and run install.sh as root
* Reboot the PI
