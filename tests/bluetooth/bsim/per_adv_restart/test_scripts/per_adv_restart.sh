#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="per_adv_restart"
exe_name=./bs_${BOARD_TS}_tests_bluetooth_bsim_per_adv_restart_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "$exe_name" -v=${verbosity_level} \
    -s="${simulation_id}" -d=0 -testid=per_adv_restart

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" -D=1 -sim_length=10e6 $@

wait_for_background_jobs
