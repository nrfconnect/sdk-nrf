#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#-------------------------------------------------------------------------------

# cpuarch.cmake is used to set things that related to the platform that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practise this is normally compiler definitions and
# variables related to hardware.


set(PLATFORM_PATH ${TARGET_PATH}/nordic_nrf)

# When we add upstream TF-M support we need to replace this with the upstream folder
# and remove the local copy of the platform. The directory structure was kept here
# to make it easier to see where it should be placed in TF-M.
set(OOT_54L_PLATFORM_PATH ${CMAKE_CURRENT_LIST_DIR}/platform/ext/target/nordic_nrf/common/nrf54l15)

include(${OOT_54L_PLATFORM_PATH}/cpuarch.cmake)
add_compile_definitions(__NRF_TFM__)
