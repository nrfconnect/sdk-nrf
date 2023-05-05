#
# Copyright (c) 2021 - 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf91 nrf91)

add_subdirectory(.. tfm_board)
