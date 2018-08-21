#!/bin/bash
echo "Tearing down fs build"
if [ -f /tmp/fs-build-active ]; then
    rm /tmp/fs-build-active
else
    echo "Build isn't up. Whatever."
fi