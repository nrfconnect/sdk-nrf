#-------------------------------------------------------------------------------
# Copyright (c) 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

# preload.cmake is used to set things that related to the platform that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practice this is normally compiler definitions and
# variables related to hardware.

include(${NRF_DIR}/modules/trusted-firmware-m/platform/ext/target/nordic_nrf/common/nrf54l15/preload.cmake)
add_compile_definitions(__NRF_TFM__)
