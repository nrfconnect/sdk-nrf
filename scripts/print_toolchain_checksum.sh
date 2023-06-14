#!/bin/bash

BASEDIR=$(dirname "$0")
REQUIREMENTS=$BASEDIR/requirements-fixed.txt
TOOLS_VERSIONS=$BASEDIR/tools-versions-linux.yml
TOOLCHAIN_VERSION=$(cat $REQUIREMENTS $TOOLS_VERSIONS | sha256sum | head -c 10)

echo "${TOOLCHAIN_VERSION}"
