#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

BOARD=nrf5340bsim/nrf5340/cpuapp
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=${ZEPHYR_NRF_MODULE_DIR}tests/bluetooth/bsim/nrf_auraconfig sysbuild=1 compile
app=${ZEPHYR_NRF_MODULE_DIR}tests/bluetooth/bsim/nrf_auraconfig/tester sysbuild=1 compile

wait_for_background_jobs
