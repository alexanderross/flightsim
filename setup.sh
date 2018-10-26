#!/bin/bash
apt-get update
apt-get --yes --force-yes install vim git-core
git config --global user.name "Steve Holt"
git config --global user.email johndoe@example.com

echo "Installing Wiring Pi"
cd /tmp
git clone git://git.drogon.net/wiringPi
cd wiringPi
git pull origin
./build

echo "Installing RF24"

cd /home/pi
git clone https://github.com/nRF24/RF24.git
cd RF24
./configure --driver=SPIDEV
sudo make install -B

echo "Installing ruby for API"
sudo apt-get --yes --force-yes install ruby2.3-dev
gem install rack-app --no-rdoc --no-ri

echo "Installing Flight Sim Drivers"
cd /home/pi
git clone https://github.com/alexanderross/flightsim.git
cd /home/pi/flightsim/src/base_controller
export FS_BASE=/home/pi/flightsim/src/base_controller
./build.sh
./install.sh