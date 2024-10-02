#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
SCRIPT_NAME=$(basename "$0")

${SCRIPT_DIR}/../_nrf_auraconfig_simulation.sh ${SCRIPT_NAME} \
  "nac" "usecase" "0"\
  "nac" "start" "0"\
  "nac_test" "test_uc0" \
