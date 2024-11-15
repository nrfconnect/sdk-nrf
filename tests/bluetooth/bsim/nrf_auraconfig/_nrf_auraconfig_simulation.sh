#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if [ -z "$1" ]; then
    echo "error: This script is not meant to be called directly"
    exit 1
fi


if [ "${ZEPHYR_BASE}" = "" ]; then
	echo "error: ZEPHYR_BASE must be set"
	exit 1
fi

SIM_ID="$1"
VERBOSITY=2

# Drop argc, instead parse for 'nac' or 'nac_test', send all that are similar to same test

# Initialize arrays to store arguments for each keyword
nac_args=()
nac_test_args=()

# Loop through the arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        nac)
            # If 'nac' is found, include it in the array and collect the following arguments
            nac_args+=("$1")
            shift
            while [[ $# -gt 0 && ! "$1" =~ ^nac$|^nac_test$ ]]; do
                nac_args+=("$1")
                shift
            done
            ;;
        nac_test)
            # If 'nac_test' is found, include it in the array and collect the following arguments
            nac_test_args+=("$1")
            shift
            while [[ $# -gt 0 && ! "$1" =~ ^nac$|^nac_test$ ]]; do
                nac_test_args+=("$1")
                shift
            done
            ;;
        *)
            # Ignore other arguments
            shift
            ;;
    esac
done

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source
cd ${BSIM_OUT_PATH}/bin

Execute ./bs_nrf5340bsim_nrf5340_cpuapp____nrf_tests_bluetooth_bsim_nrf_auraconfig_prj_conf \
  -v=${VERBOSITY} -s=${SIM_ID} -d=0 -RealEncryption=1 -cpu0_testid=nac -argstest ${nac_args[@]}

Execute ./bs_nrf5340bsim_nrf5340_cpuapp____nrf_tests_bluetooth_bsim_nrf_auraconfig_tester_prj_conf \
  -v=${VERBOSITY} -s=${SIM_ID} -d=1 -RealEncryption=1 -cpu0_testid=nac_test \
  -argstest ${nac_test_args[@]}

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY} -s=${SIM_ID} -D=2 -sim_length=2e6

wait_for_background_jobs # Wait for all programs in background and return != 0 if any fails
