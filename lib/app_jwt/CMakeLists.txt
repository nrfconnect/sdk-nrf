#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
    app_jwt.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
