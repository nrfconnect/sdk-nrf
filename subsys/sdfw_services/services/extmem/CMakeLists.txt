#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(extmem_remote.c)

generate_and_add_cbor_files(extmem_service.cddl zcbor_generated
  extmem_req
  extmem_rsp
  extmem_nfy
)
