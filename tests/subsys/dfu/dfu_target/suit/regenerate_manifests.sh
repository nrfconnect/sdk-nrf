#!/bin/bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Script to regenerate SUIT manifests for DFU target tests.
# To use this script you must have the following tools installed:
# - suit-generator
# - zcbor
#
# The script will update the following files:
# - dfu_cache_partition_1.c which contains the binary cache as C code
# - manifest.c which contains the signed manifest as C code
#
# Before using this script paste the following files into
# the nrf/tests/subsys/suit/manifest_common directory:
# - file.bin, the testing binary file.


cd ../../../suit/manifest_common

### Generate testing cache file ###
./generate_dfu_cache.sh file.bin

### Generate Manifests ###
./regenerate.sh ../../dfu/dfu_target/suit/manifests/manifest_root.yaml
./regenerate.sh ../../dfu/dfu_target/suit/manifests/manifest_app.yaml
python3 replace_manifest.py ../../dfu/dfu_target/suit/src/manifest.c sample_signed.suit
./regenerate.sh ../../dfu/dfu_target/suit/manifests/manifest_with_payload.yaml
python3 replace_manifest.py ../../dfu/dfu_target/suit/src/manifest_with_payload.c sample_signed.suit

### Generate Cache Partition ###
python3 replace_manifest.py ../../dfu/dfu_target/suit/src/dfu_cache_partition.c sample_cache.bin
