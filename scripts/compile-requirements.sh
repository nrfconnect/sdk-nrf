#!/usr/bin/env bash

# Check if an argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <path for output file>"
    exit 1
fi

# Store the path provided as the first argument
OUT_FILE="$1"

# Check if the path is absolute or relative
if [[ "$OUT_FILE" == /* ]]; then
    echo "Absolute path provided: $OUT_FILE"
else
    echo "Relative path provided: $PWD/$OUT_FILE"
    OUT_FILE="$PWD/$OUT_FILE"
fi
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

pip-compile \
    --build-isolation \
    --strip-extras \
    --annotation-style line \
    --allow-unsafe \
    --output-file $OUT_FILE \
    ./bootloader/mcuboot/scripts/requirements.txt \
    ./zephyr/scripts/requirements.txt  \
    ./nrf/scripts/requirements.txt \
    ./nrf/scripts/requirements-ci.txt \
    ./nrf/scripts/requirements-extra.txt

deactivate
rmvirtualenv pip-fixed-venv
