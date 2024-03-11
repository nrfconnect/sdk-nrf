#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(sdfw_update_service.c)

generate_and_add_cbor_files(sdfw_update_service.cddl zcbor_generated
  sdfw_update_req
  sdfw_update_rsp
)
