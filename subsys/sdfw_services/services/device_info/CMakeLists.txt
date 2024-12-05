#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(device_info_service.c)

generate_and_add_cbor_files(device_info_service.cddl zcbor_generated
  device_info_service_req
  device_info_service_rsp
)
