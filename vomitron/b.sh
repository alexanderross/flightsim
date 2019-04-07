#!/bin/sh
version=$(git rev-parse HEAD | cut -c 1-6)
docker build . -t alexanderross/vomitron:$version
docker push alexanderross/vomitron:$version
