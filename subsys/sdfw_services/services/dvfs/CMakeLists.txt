#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(dvfs_service.c)

generate_and_add_cbor_files(dvfs_service.cddl zcbor_generated
  dvfs_oppoint_req
  dvfs_oppoint_rsp
)
