#!/bin/bash

sudo apt-get -q -y update
sudo apt-get -q -y install hostapd dnsmasq

sudo systemctl unmask hostapd
sudo systemctl disable hostapd dnsmasq

sudo cp /home/pi/shuttle-panels/autohotspot/hostapd.conf /etc/hostapd/hostapd.conf
sudo cp /home/pi/shuttle-panels/autohotspot/hostapd.default /etc/default/hostapd
sudo cp /home/pi/shuttle-panels/autohotspot/dnsmasq.conf /etc/dnsmasq.d/autohotspot

sudo cp /home/pi/shuttle-panels/autohotspot/dhcpcd.conf /etc/dhcpcd.conf

sudo cp /home/pi/shuttle-panels/autohotspot/autohotspot.service /etc/systemd/system/autohotspot.service
sudo cp /home/pi/shuttle-panels/autohotspot/autohotspot.sh /usr/bin/autohotspot
sudo systemctl enable autohotspot.service
