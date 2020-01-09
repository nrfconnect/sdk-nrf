#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory(provision)
add_subdirectory(bl_validation)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_CRYPTO bl_crypto)

zephyr_include_directories(include)
