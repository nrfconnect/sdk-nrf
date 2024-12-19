#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

INCR_BUILD=1

BOARD=nrf5340bsim/nrf5340/cpuapp

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app_root=${ZEPHYR_BASE}/../nrf/ app=tests/bluetooth/iso/bsim sysbuild=1 compile

wait_for_background_jobs
