#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
SCRIPT_NAME=$(basename "$0")

${SCRIPT_DIR}/../iso_and_acl.sh ${SCRIPT_NAME} \
  "iso_brcast_src" "rtn_set" "2" \
  "acl_central" "send_int_set" "1000" \
  "acl_central" "payload_size_set" "20" \
  "tester" "acl_fail_limit_percent" "0" \
  "tester" "iso_fail_limit_percent" "1" \
  "tester" "acl_packets_limit_min" "3" \
  "tester" "iso_packets_limit_min" "800"
