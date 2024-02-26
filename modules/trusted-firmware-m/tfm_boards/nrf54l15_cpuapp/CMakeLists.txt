#-------------------------------------------------------------------------------
# Copyright (c) 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

set(NRF_BOARD_SELECTED True)

set(OOT_PLATFORM_PATH ${NRF_DIR}/modules/trusted-firmware-m/platform/ext/target/nordic_nrf)

add_subdirectory(${OOT_PLATFORM_PATH}/common/nrf54l15 nrf54l15)

add_subdirectory(.. tfm_board)
