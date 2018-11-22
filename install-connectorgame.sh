#!/bin/bash

sudo apt-get -q -y update
sudo apt-get -q -y install wiringpi scons git lighttpd php7.0-fpm libasound2-dev fuse lsof

sudo systemctl stop lighttpd php7.0-fpm

git clone --recursive git://github.com/willemm/shuttle-panels.git /home/pi/shuttle-panels

sudo chown -R pi.pi /var/log/lighttpd

sudo cp /home/pi/shuttle-panels/asound.conf /etc/asound.conf
sudo cp /home/pi/shuttle-panels/boot-config.txt /boot/config.txt
sudo cp /home/pi/shuttle-panels/lighttpd.conf /etc/lighttpd/lighttpd.conf
cat /home/pi/shuttle-panels/fpm-www.conf | sudo tee -a /etc/php/7.0/fpm/pool.d/www.conf

make -C /home/pi/shuttle-panels/connectorgame install

sudo systemctl start lighttpd php7.0-fpm
