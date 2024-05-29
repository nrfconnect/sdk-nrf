#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lc3_decoder.c)

target_include_directories(app PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/include/audio_module
	${ZEPHYR_NRF_MODULE_DIR}/include/audio_modules
)

zephyr_include_directories(
	${ZEPHYR_BASE}/../nrfxlib/lc3/codec/inc
    ${ZEPHYR_BASE}/../nrfxlib/lc3/platform/os/inc
    ${ZEPHYR_BASE}/../nrfxlib/lc3/platform/os/baremetal/inc
)
