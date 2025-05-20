#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This boilerplate is automatically included through ZephyrBuildConfig.cmake, found in
# ${NRF_DIR}/share/zephyrbuild-package/cmake/ZephyrBuildConfig.cmake
# For more information regarding the Zephyr Build Configuration CMake package, please refer to:
# https://docs.nordicsemi.com/bundle/ncs-latest/page/zephyr/build/zephyr_cmake_package.html#zephyr_build_configuration_cmake_packages

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

list(PREPEND CMAKE_MODULE_PATH ${NRF_DIR}/cmake/modules)
