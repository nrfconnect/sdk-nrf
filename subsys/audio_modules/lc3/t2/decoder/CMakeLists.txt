#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lc3_decoder.c)

set(LC3_CODEC_DIR "${ZEPHYR_BASE}/../nrfxlib/lc3")

zephyr_include_directories(
	${ZEPHYR_BASE}/../nrfxlib/lc3/codec/inc
    ${ZEPHYR_BASE}/../nrfxlib/lc3/platform/os/inc
    ${ZEPHYR_BASE}/../nrfxlib/lc3/platform/os/baremetal/inc
)
