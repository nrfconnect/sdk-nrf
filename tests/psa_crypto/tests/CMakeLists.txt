#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
zephyr_sources(test_ikg_identity_key_sign_verify.c)
zephyr_sources(test_ikg_key_derivation.c)
zephyr_sources(test_kmu_write.c)
zephyr_sources(test_kmu_use.c)
