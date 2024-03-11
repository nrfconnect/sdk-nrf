#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(echo_service.c)

generate_and_add_cbor_files(echo_service.cddl zcbor_generated
  echo_service_req
  echo_service_rsp
)
