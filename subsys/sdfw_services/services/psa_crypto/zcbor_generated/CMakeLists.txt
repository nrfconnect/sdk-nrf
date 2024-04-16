#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# Generated using cmake macro generate_and_add_cbor_files.
#

zephyr_sources(
	psa_crypto_service_decode.c
	psa_crypto_service_encode.c
)

zephyr_include_directories(.)
