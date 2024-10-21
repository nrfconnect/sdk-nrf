#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
SCRIPT_NAME=$(basename "$0")

${SCRIPT_DIR}/../iso_and_acl.sh ${SCRIPT_NAME} \
  "iso_brcast_src" "rtn_set" "3" \
  "acl_central" "send_int_set" "100" \
  "acl_central" "payload_size_set" "20" \
  "tester" "acl_fail_limit_percent" "18" \
  "tester" "iso_fail_limit_percent" "1" \
  "tester" "acl_packets_limit_min" "10" \
  "tester" "iso_packets_limit_min" "700"
