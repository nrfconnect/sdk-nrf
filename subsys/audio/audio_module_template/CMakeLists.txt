#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/audio_module_template.c)

target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/audio_module
                     ${ZEPHYR_NRF_MODULE_DIR}/include/audio)
