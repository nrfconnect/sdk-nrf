#!/bin/bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

SUIT_GENERATOR_DIR="../../../../../modules/lib/suit-generator"
if [ -z "$1" ]
  then
    echo "generate_dfu_cache.sh: path to bin file required"
    exit -1
fi

### DFU cache generation ###
echo "Generating DFU cache for $1 input file ..."
suit-generator cache_create --input "#file.bin,file.bin" --output-file sample_cache.bin
