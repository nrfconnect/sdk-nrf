#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
SCRIPT_NAME=$(basename "$0")

source ${SCRIPT_DIR}/_usecase_defines.sh

${SCRIPT_DIR}/../_nrf_auraconfig_simulation.sh ${SCRIPT_NAME} \
  "nac" "preset" "24_2_1" ${BIG_0}\
  "nac" "adv_name" "'Lecture hall'" ${BIG_0}\
  "nac" "broadcast_name" "Lecture" ${BIG_0}\
  "nac" "broadcast_id" "fixed" ${BIG_0} "0x123456"\
  "nac" "packing" "int" ${BIG_0}\
  "nac" "lang" "eng" ${BIG_0} ${SUB_G_0}\
  "nac" "context" "live" ${BIG_0} ${SUB_G_0}\
  "nac" "program_info" "'Mathematics 101'" ${BIG_0} ${SUB_G_0}\
  "nac" "immediate" ${TRUE} ${BIG_0} ${SUB_G_0}\
  "nac" "num_bises" "1" ${BIG_0} ${SUB_G_0}\
  "nac" "start" ${BIG_0}\
  "nac_test" "adv_name" "'Lecture hall'"\
  "nac_test" "broadcast_name" "Lecture"\
  "nac_test" "broadcast_id" "0x123456"\
  "nac_test" "phy" ${PHY_2M}\
  "nac_test" "pd_us" "40000"\
  "nac_test" "encryption" ${FALSE}\
  "nac_test" "broadcast_code" "blank"\
  "nac_test" "num_subgroups" "1"\
  "nac_test" "std_quality" ${TRUE}\
  "nac_test" "high_quality" ${FALSE}\
  "nac_test" "sampling_rate" "24000" ${SUB_G_0}\
  "nac_test" "frame_duration_us" "10000" ${SUB_G_0}\
  "nac_test" "octets_per_frame" "60" ${SUB_G_0}\
  "nac_test" "lang" "eng" ${SUB_G_0}\
  "nac_test" "immediate" ${TRUE} ${SUB_G_0}\
  "nac_test" "context" "live" ${SUB_G_0}\
  "nac_test" "program_info" "'Mathematics 101'" ${SUB_G_0}\
  "nac_test" "num_bis" "1" ${SUB_G_0}\
  "nac_test" "location" ${MA} ${SUB_G_0} ${BIS_0}\
  "nac_test" "run_test"\
