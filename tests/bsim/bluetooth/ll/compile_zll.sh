#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${NRF_BASE:?NRF_BASE must be set to point to the sdk-nrf root directory}"

source ${NRF_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/conn conf_file=prj_split.conf snippet=bt-ll-sw-split compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_1ms.conf snippet=bt-ll-sw-split compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_tx_defer.conf snippet=bt-ll-sw-split compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_privacy.conf snippet=bt-ll-sw-split compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_low_lat.conf snippet=bt-ll-sw-split compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_single_timer.conf snippet=bt-ll-sw-split compile

app=tests/bsim/bluetooth/ll/multiple_id snippet=bt-ll-sw-split compile

app=tests/bsim/bluetooth/ll/throughput snippet=bt-ll-sw-split compile
app=tests/bsim/bluetooth/ll/throughput conf_overlay=overlay-no_phy_update.conf \
snippet=bt-ll-sw-split compile

wait_for_background_jobs
