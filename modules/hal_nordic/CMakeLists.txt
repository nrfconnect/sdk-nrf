# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_include_directories_ifdef(CONFIG_HAS_NRFX .)
add_subdirectory_ifdef(CONFIG_HAS_NRFX nrfx)
add_subdirectory(${ZEPHYR_BASE}/modules/hal_nordic ${CMAKE_CURRENT_BINARY_DIR}/modules/hal_nordic)
