#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(ib_crypto_client.c)
zephyr_link_libraries(nrfxlib_crypto)
zephyr_include_directories(../include)
zephyr_include_directories(../ib_crypto)
