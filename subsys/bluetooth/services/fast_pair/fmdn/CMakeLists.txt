#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fmdn)

target_include_directories(fmdn PUBLIC include)
target_include_directories(fmdn PRIVATE include_priv)
target_include_directories(fmdn PRIVATE ../include/common)
target_link_libraries(fmdn PRIVATE fp_crypto)
target_link_libraries(fmdn PRIVATE fp_storage)
