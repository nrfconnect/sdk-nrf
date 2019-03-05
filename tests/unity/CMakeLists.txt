#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(CMOCK_DIR ${ZEPHYR_BASE}/../cmock)

zephyr_include_directories(
	${CMOCK_DIR}/vendor/unity/src
	${CMOCK_DIR}/src
)

zephyr_sources(
	${CMOCK_DIR}/vendor/unity/src/unity.c
	${CMOCK_DIR}/src/cmock.c
)