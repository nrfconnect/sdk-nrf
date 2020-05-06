#!/usr/bin/env bash

# This script merges the py-reqs from mcuboot, zephyr, and nrf.
# Then generates a frozen requirments-ci.txt for tests to run with.

#OPTIONAL
OUT_FILE=$1
if [ -z "$OUT_FILE" ]; then
    OUT_FILE=nrf/scripts/requirements-ci.txt
fi
echo "Writing frozen requirements to: $OUT_FILE"


TOPDIR=$(west topdir)
cd $TOPDIR


source ~/.local/bin/virtualenvwrapper.sh

rmvirtualenv pip-freeze-ci-venv
mkvirtualenv pip-freeze-ci-venv
workon pip-freeze-ci-venv


# Convert static ci-tools req to a floating tools list
sed 's/[>=].*//' tools/ci-tools/requirements.txt > tools/ci-tools/requirements-float.txt

pip3 install --isolated \
    -r bootloader/mcuboot/scripts/requirements.txt \
    -r zephyr/scripts/requirements.txt  \
    -r tools/ci-tools/requirements-float.txt  \
    -r nrf/scripts/requirements.txt
pip3 freeze > $OUT_FILE

deactivate
rmvirtualenv pip-freeze-ci-venv
