#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/audio_module.c
)

target_sources_ifdef(CONFIG_AUDIO_MODULE_TEMPLATE_TEST app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/audio_module_template.c)

target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/audio_module)
