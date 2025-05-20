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
  "nac" "num_subgroups" "2" ${BIG_0}\
  "nac" "preset" "24_2_2" ${BIG_0}\
  "nac" "adv_name" "'Brians laptop'" ${BIG_0}\
  "nac" "broadcast_name" "'Personal multi-lang.'" ${BIG_0}\
  "nac" "packing" "int" ${BIG_0}\
  "nac" "encrypt" ${TRUE} ${BIG_0} "Auratest"\
  "nac" "lang" "eng" ${BIG_0} ${SUB_G_0}\
  "nac" "lang" "chi" ${BIG_0} ${SUB_G_1}\
  "nac" "context" "media" ${BIG_0} ${SUB_G_0}\
  "nac" "context" "media" ${BIG_0} ${SUB_G_1}\
  "nac" "num_bises" "2" ${BIG_0} ${SUB_G_0}\
  "nac" "num_bises" "2" ${BIG_0} ${SUB_G_1}\
  "nac" "location" "FL" ${BIG_0} ${SUB_G_0} ${BIS_0}\
  "nac" "location" "FR" ${BIG_0} ${SUB_G_0} ${BIS_1}\
  "nac" "location" "FL" ${BIG_0} ${SUB_G_1} ${BIS_0}\
  "nac" "location" "FR" ${BIG_0} ${SUB_G_1} ${BIS_1}\
  "nac" "rtn" "2" ${BIG_0} ${SUB_G_0}\
  "nac" "rtn" "2" ${BIG_0} ${SUB_G_1}\
  "nac" "start"\
  "nac_test" "adv_name" "'Brians laptop'"\
  "nac_test" "broadcast_name" "'Personal multi-lang.'"\
  "nac_test" "phy" ${PHY_2M}\
  "nac_test" "pd_us" "40000"\
  "nac_test" "encryption" ${TRUE}\
  "nac_test" "broadcast_code" "Auratest"\
  "nac_test" "num_subgroups" "2"\
  "nac_test" "std_quality" ${TRUE}\
  "nac_test" "high_quality" ${FALSE}\
  "nac_test" "sampling_rate" "24000" ${SUB_G_0}\
  "nac_test" "sampling_rate" "24000" ${SUB_G_1}\
  "nac_test" "frame_duration_us" "10000" ${SUB_G_0}\
  "nac_test" "frame_duration_us" "10000" ${SUB_G_1}\
  "nac_test" "octets_per_frame" "60" ${SUB_G_0}\
  "nac_test" "octets_per_frame" "60" ${SUB_G_1}\
  "nac_test" "lang" "eng" ${SUB_G_0}\
  "nac_test" "lang" "chi" ${SUB_G_1}\
  "nac_test" "context" "media" ${SUB_G_0}\
  "nac_test" "context" "media" ${SUB_G_1}\
  "nac_test" "num_bis" "2" ${SUB_G_0}\
  "nac_test" "num_bis" "2" ${SUB_G_1}\
  "nac_test" "location" ${FL} ${SUB_G_0} ${BIS_0}\
  "nac_test" "location" ${FR} ${SUB_G_0} ${BIS_1}\
  "nac_test" "location" ${FL} ${SUB_G_1} ${BIS_0}\
  "nac_test" "location" ${FR} ${SUB_G_1} ${BIS_1}\
  "nac_test" "run_test"\
