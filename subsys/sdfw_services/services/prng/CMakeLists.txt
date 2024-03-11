#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(prng_service.c)

generate_and_add_cbor_files(prng_service.cddl zcbor_generated
  prng_req
  prng_rsp
)
