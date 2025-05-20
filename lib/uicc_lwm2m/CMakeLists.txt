#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_include_directories(
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/
)

zephyr_library_sources(
    asn1_decode.c
    pkcs15_decode.c
    uicc_lwm2m.c
)
