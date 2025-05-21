#!/bin/bash

echo "Applying udev rule for /dev/uinput..."

echo "Coppying udev rule file"
sudo cp data/udev/99-uinput.rules /etc/udev/rules.d
echo "Reloading udev rules"
sudo udevadm control --reload-rules
sudo udevadm trigger

echo "Done"
