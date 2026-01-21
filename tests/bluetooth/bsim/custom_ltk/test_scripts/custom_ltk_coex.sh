#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="custom_ltk_coex"
exe_name=./bs_${BOARD_TS}_tests_bluetooth_bsim_custom_ltk_prj_conf

cd ${BSIM_OUT_PATH}/bin

# Test custom LTK coexistence with SMP
Execute "$exe_name" -v=${verbosity_level} \
    -s="${simulation_id}" -d=0 -testid=central_ltk_coex_test -RealEncryption=1

Execute "$exe_name" -v=${verbosity_level} \
    -s="${simulation_id}" -d=1 -testid=peripheral_ltk_coex_test -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=10e6 $@

wait_for_background_jobs
