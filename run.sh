#!/bin/bash
# run.sh
PLATFORM=${PLATFORM:-"linux/amd64"}
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

docker run --platform $PLATFORM \
    -v "$(pwd)":/app \
    sos-sdp "$@"