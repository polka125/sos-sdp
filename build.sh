#!/bin/bash

echo "Installing pythog dependencies"
pip install -r requirements.txt

echo "Building docker image"
docker build --platform linux/amd64 -t sos-sdp .

echo "Done"