#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This script is not meant to be called directly.
# It should rather be invoked by the scripts in the tests folder, which in turn may be called
# by the run_parallel.sh script in the Zephyr bsim folder.

if [ -z "$1" ]; then
    echo "error: This script is not meant to be called directly"
    exit 1
fi

SIM_ID="$1"
VERBOSITY=2

if [ "${ZEPHYR_BASE}" = "" ]; then
	echo "error: ZEPHYR_BASE must be set"
	exit 1
fi

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source
cd ${BSIM_OUT_PATH}/bin

printf "\n Starting Babblesim: %s test: %s \n" "$0" "$SIM_ID"

Execute ./bs_nrf5340bsim_nrf5340_cpuapp_tests_bluetooth_iso_bsim_prj_conf \
    -v=${VERBOSITY} -s=${SIM_ID} -d=0 -rs=22 -cpu0_testid=bis_and_acl \
    -argstest "tester" "dev" CENTRAL_SOURCE "${@:2}"

Execute ./bs_nrf5340bsim_nrf5340_cpuapp_tests_bluetooth_iso_bsim_prj_conf \
    -v=${VERBOSITY} -s=${SIM_ID} -d=1 -rs=23 -cpu0_testid=bis_and_acl \
    -argstest "tester" "dev" PERIPHERAL_SINK "${@:2}"

Execute ./bs_nrf5340bsim_nrf5340_cpuapp_tests_bluetooth_iso_bsim_prj_conf \
    -v=${VERBOSITY} -s=${SIM_ID} -d=2 -rs=24 -cpu0_testid=bis_and_acl \
    -argstest "tester" "dev" PERIPHERAL_SINK "${@:2}"

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY} -s=${SIM_ID} \
    -D=3 -sim_length=14e6

wait_for_background_jobs
