#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING fault.c)
