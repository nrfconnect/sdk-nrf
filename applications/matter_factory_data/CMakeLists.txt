#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Do not apply shield or snippets for the factory data partition and use an empty file for the
# board defconfig file
set(BOARD_DEFCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf)
set(CACHED_SHIELD "" CACHE STRING "" FORCE)
set(CACHED_SNIPPET "" CACHE STRING "" FORCE)

find_package(Zephyr
  COMPONENTS zephyr_default:kconfig
  REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

project(matter_factory_data)

# Manually include board configuration to enable automatic runners.yaml generation
include(${BOARD_DIR}/board.cmake OPTIONAL)

# Create the runners_yaml_props_target that flash system expects
add_custom_target(runners_yaml_props_target)

# Override the runners.yaml path to use CMAKE_CURRENT_BINARY_DIR/zephyr instead of
# PROJECT_BINARY_DIR, this ensures runners.yaml is generated at <build>/softdevice/zephyr where
# west expects it
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

# Import Kconfig and DTS from the application then generate the factory data hex file which will
# be flashed when west flash is invoked. Unlike PM, it will not be merged with the application
# into a new hex file
set(app_dir ${CMAKE_CURRENT_BINARY_DIR}/../${CONFIG_APPLICATION_NAME})
import_kconfig(CONFIG_ ${app_dir}/zephyr/.config)
zephyr_dt_import(EDT_PICKLE_FILE ${app_dir}/zephyr/edt.pickle TARGET devicetree_target)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${app_dir}/zephyr/.config
  ${app_dir}/zephyr/edt.pickle
)
set(CHIP_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR})
include(/tmp/bb/modules/lib/matter/config/nrfconnect/chip-module/generate_factory_data.cmake)
nrfconnect_generate_factory_data()

# Set hex_file property to point to factory data output file in this directory
set_target_properties(runners_yaml_props_target PROPERTIES
  hex_file "${CMAKE_CURRENT_BINARY_DIR}/factory_data.hex"
)

# Include flash support to automatically generate runners.yaml
include(${ZEPHYR_BASE}/cmake/flash/CMakeLists.txt)
