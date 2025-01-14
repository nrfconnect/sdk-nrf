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
  "nac" "adv_name" "'Silent TV1'" ${BIG_0}\
  "nac" "broadcast_name" "TV-audio" ${BIG_0}\
  "nac" "packing" "int" ${BIG_0}\
  "nac" "encrypt" ${TRUE} ${BIG_0} "Auratest"\
  "nac" "context" "media" ${BIG_0} ${SUB_G_0}\
  "nac" "program_info" "News" ${BIG_0} ${SUB_G_0}\
  "nac" "immediate" ${TRUE} ${BIG_0} ${SUB_G_0}\
  "nac" "num_bises" "2" ${BIG_0} ${SUB_G_0}\
  "nac" "location" "FL" ${BIG_0} ${SUB_G_0} ${BIS_0}\
  "nac" "location" "FR" ${BIG_0} ${SUB_G_0} ${BIS_1}\
  "nac" "start" ${BIG_0}\
  "nac_test" "adv_name" "'Silent TV1'"\
  "nac_test" "broadcast_name" "TV-audio"\
  "nac_test" "phy" ${PHY_2M}\
  "nac_test" "pd_us" "40000"\
  "nac_test" "encryption" "${TRUE}"\
  "nac_test" "broadcast_code" "Auratest"\
  "nac_test" "num_subgroups" "1"\
  "nac_test" "std_quality" ${TRUE}\
  "nac_test" "high_quality" ${FALSE}\
  "nac_test" "sampling_rate" "24000" ${SUB_G_0}\
  "nac_test" "frame_duration_us" "10000" ${SUB_G_0}\
  "nac_test" "octets_per_frame" "60" ${SUB_G_0}\
  "nac_test" "immediate" ${TRUE} ${SUB_G_0}\
  "nac_test" "context" "media" ${SUB_G_0}\
  "nac_test" "program_info" "News" ${SUB_G_0}\
  "nac_test" "num_bis" "2" ${SUB_G_0}\
  "nac_test" "location" ${FL} ${SUB_G_0} ${BIS_0}\
  "nac_test" "location" ${FR} ${SUB_G_0} ${BIS_1}\
  "nac_test" "run_test"\
