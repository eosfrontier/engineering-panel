#!/bin/bash

# INstall packages
apt-get install fuse lsof

# Swap off
dphys-swapfile swapoff
dphys-swapfile uninstall 
update-rc.d dphys-swapfile disable
systemctl disable dphys-swapfile

# Install scripts
cp mount_overlay /usr/local/bin/
cp saveoverlays /etc/init.d/
cp rootro /usr/local/bin/
ln -s rootro /usr/local/bin/rootrw
cp syncoverlayfs.service /lib/systemd/system/

# Disable fake-hwclock
systemctl disable fake-hwclock.service
systemctl stop fake-hwclock.service
mv /etc/fake-hwclock.data /var/log/fake-hwclock.data
ln -s /var/log/fake-hwclock.data /etc/fake-hwclock.data

# Enable service
systemctl daemon-reload
systemctl enable syncoverlayfs.service

# Change boot cmdline
perl -pi -e 's{rootwait( noswap fastboot ro)?}{rootwait noswap fastboot ro}' /boot/cmdline.txt

# Edit /etc/fstab
perl -pi -e 's{(/(?:boot?)\s*vfat\s*defaults)}{$1,ro}' /etc/fstab
cat fstab.add >> /etc/fstab

for D in /home /var
do
    mv -v ${D} ${D}_org
    cd ${D}_org
    find . | cpio -pdum ${D}_stage
    mkdir -v ${D} ${D}_rw ${D}/.overlaysync ${D}_org/.overlaysync
done
mount /home
mount /var
