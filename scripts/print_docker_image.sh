#!/bin/bash

BASEDIR=$(dirname "$0")
TOOLCHAIN_VERSION=$($BASEDIR/print_toolchain_checksum.sh)
DOCKER_VERSION=$(cat $BASEDIR/ncs-docker-version.txt)

echo "docker-dtr.nordicsemi.no/sw-production/ncs-build:${TOOLCHAIN_VERSION}_${DOCKER_VERSION}"
