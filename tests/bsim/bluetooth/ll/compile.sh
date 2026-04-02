#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${NRF_BASE:?NRF_BASE must be set to point to the sdk-nrf root directory}"

source ${NRF_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/conn conf_file=prj_split.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_1ms.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_tx_defer.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_privacy.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_low_lat.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_single_timer.conf compile

app=tests/bsim/bluetooth/ll/multiple_id compile

app=tests/bsim/bluetooth/ll/throughput compile
app=tests/bsim/bluetooth/ll/throughput conf_overlay=overlay-no_phy_update.conf compile

wait_for_background_jobs
