#
# Copyright (c) 2025, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(PLATFORM_PATH platform/ext/target/nordic_nrf)

include(${PLATFORM_PATH}/common/nrf7120/cpuarch.cmake)
add_compile_definitions(__NRF_TFM__)
