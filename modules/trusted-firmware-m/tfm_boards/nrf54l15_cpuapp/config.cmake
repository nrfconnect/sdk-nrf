#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/../common/config.cmake)

set(NRF_SOC_VARIANT nrf54l15 CACHE STRING "nRF SoC Variant")

include(${PLATFORM_PATH}/common/${NRF_SOC_VARIANT}/config.cmake)
