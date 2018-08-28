#!/bin/bash

touch /tmp/fs-build-active
export FS_BASE=/home/pi/flightsim/src/base_controller

echo "Compiling GPIO panel executable"
gcc $FS_BASE/gpioctrl.c -o $FS_BASE/gpio-panel -lwiringPi

echo "Compiling Serial Comm executable"
gcc $FS_BASE/ser2pc.c -o $FS_BASE/ser-comm

echo "Compiling RF Comm executable"
#gcc $FS_BASE/contrf24.cpp -o $FS_BASE/rf24-comm

