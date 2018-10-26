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

echo "Installing RF24"
echo "Checking for SPI enabled"
if [ -f /dev/spidev0.0 ]; then
	echo "SPI is enabled. Cool."
else
  echo "SPI not enabled - use 'sudo raspi-config' to enable and re-run"
  exit 1
fi

cd /home/pi
git clone https://github.com/nRF24/RF24.git
cd RF24
./configure --driver=SPIDEV
sudo make install -B

echo "Installing ruby for API"
sudo apt-get install ruby2.3-dev
gem install rack-app

echo "Installing Flight Sim Drivers"
cd /home/pi
git clone https://github.com/alexanderross/flightsim.git
cd /home/pi/flightsim/src/base_controller
export FS_BASE=/home/pi/flightsim/src/base_controller
./build.sh
./install.sh