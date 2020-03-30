#!/usr/bin/env sh


OUT_FILE=$1
if [ -z "$OUT_FILE" ]; then
    OUT_FILE=nrf/scripts/requirements-ci.txt
fi
echo "Writing frozen requirements to: $OUT_FILE"



TOPDIR=$(west topdir)
cd $TOPDIR


pip3 install -r nrf/scripts/requirements.txt     \
            -r zephyr/scripts/requirements.txt  \
            -r mcuboot/scripts/requirements.txt

pip3 freeze > $OUT_FILE
