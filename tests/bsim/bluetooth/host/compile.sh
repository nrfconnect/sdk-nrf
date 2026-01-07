#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

: "${NRF_BASE:?NRF_BASE must be set to point to the sdk-nrf root directory}"

#Set a default value to BOARD if it does not have one yet
BOARD="${BOARD:-nrf52_bsim/native}"

west twister -T ${NRF_BASE}/tests/bsim/bluetooth/host/ -p ${BOARD}
