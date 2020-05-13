#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# This boilerplate is automatically included through ZephyrBuildConfig.cmake, found in
# ${NRF_DIR}/share/zephyrbuild-package/cmake/ZephyrBuildConfig.cmake
# For more information regarding the Zephyr Build Configuration CMake package, please refer to:
# https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/guides/zephyr_cmake_package.html#zephyr-build-configuration-cmake-package

include(${NRF_DIR}/boards/deprecated.cmake)

if(NOT BOARD)
        set(BOARD $ENV{BOARD})
endif()

# Check if selected board is supported.
if(DEFINED NRF_SUPPORTED_BOARDS)
        if(NOT BOARD IN_LIST NRF_SUPPORTED_BOARDS)
                message(FATAL_ERROR "board ${BOARD} is not supported")
        endif()
endif()

# Check if selected build type is supported.
if(DEFINED NRF_SUPPORTED_BUILD_TYPES)
        if(NOT CMAKE_BUILD_TYPE IN_LIST NRF_SUPPORTED_BUILD_TYPES)
                message(FATAL_ERROR "${CMAKE_BUILD_TYPE} variant is not supported")
        endif()
endif()

# Add NRF_DIR as a BOARD_ROOT in case the board is in NRF_DIR
list(APPEND BOARD_ROOT ${NRF_DIR})

# Add NRF_DIR as a DTS_ROOT to include nrf/dts
list(APPEND DTS_ROOT ${NRF_DIR})
