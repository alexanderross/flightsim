#!/bin/bash
apt-get update
apt-get install vim
apt-get install git-core
git config --global user.name "John Doe"
git config --global user.email johndoe@example.com

echo "Installing Wiring Pi"
cd /tmp
git clone git://git.drogon.net/wiringPi
cd wiringPi
git pull origin
./build

echo "Installing Flight Sim Drivers"
cd /home/pi
git clone https://github.com/alexanderross/flightsim.git
cd /home/pi/flightsim/src/base_controller
export FS_BASE=/home/pi/flightsim/src/base_controller
./build.sh
./install.sh