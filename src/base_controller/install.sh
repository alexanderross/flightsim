#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root, what for all the systemd stuff" 
   exit 1
fi

export FS_BASE=/home/pi/flightsim/src/base_controller

install_fs_svc () {
	echo "Installing $1 service"
  cp $FS_BASE/systemd/$1.service /lib/systemd/system/
  chmod u+x /lib/systemd/system/$1.service
  systemctl enable $1
}

install_fs_svc fs-panel
#install_fs_svc fs-ser-comm
#install_fs_svc fs-rf24-comm