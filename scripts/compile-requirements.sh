#!/usr/bin/env bash

# This script merges the py-reqs from mcuboot, zephyr, and nrf.

OUT_FILE=$1
[ -z "$OUT_FILE" ] && echo "Error output file not provided" && exit 1
echo "Writing frozen requirements to: $OUT_FILE"
echo "Log python version: $(python --version)"

TOPDIR=$(west topdir)
cd $TOPDIR

source ~/.local/bin/virtualenvwrapper.sh
[[ $? != 0 ]] && echo "error sourcing virtualenvwrapper" && exit 1


rmvirtualenv pip-fixed-venv > /dev/null 2>&1
mkvirtualenv pip-fixed-venv  > /dev/null 2>&1
workon pip-fixed-venv  > /dev/null 2>&1
pip3 install pip-tools > /dev/null 2>&1
pip3 install setuptools --upgrade


pip-compile --allow-unsafe --output-file $OUT_FILE --strip-extras \
    ./bootloader/mcuboot/scripts/requirements.txt \
    ./zephyr/scripts/requirements.txt  \
    ./nrf/scripts/requirements.txt \
    ./nrf/scripts/requirements-ci.txt \
    ./nrf/scripts/requirements-extra.txt
sed '/^$/d' -i $OUT_FILE
deactivate
rmvirtualenv pip-fixed-venv
