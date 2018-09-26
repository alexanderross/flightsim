#!/bin/bash

touch /tmp/fs-build-active
export FS_BASE=/home/pi/flightsim/src/base_controller

echo "Compiling GPIO panel executable"
gcc $FS_BASE/gpioctrl.c -o $FS_BASE/gpio-panel -lwiringPi

echo "Compiling Serial Comm executable"
gcc $FS_BASE/ser2pc.c -g -o $FS_BASE/ser-comm

echo "Compiling RF Comm executable"
g++ $FS_BASE/rfaxiscomm.cpp -lrf24 -o $FS_BASE/rf24-comm

echo "Compiling debugging memory writer"
gcc $FS_BASE/writesharedmem.c -o memwt

