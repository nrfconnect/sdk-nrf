#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# Generated using cmake macro generate_and_add_cbor_files.
#

zephyr_sources(
	reset_evt_service_decode.c
	reset_evt_service_encode.c
)

zephyr_include_directories(.)
