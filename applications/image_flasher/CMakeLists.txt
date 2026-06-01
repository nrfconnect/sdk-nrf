#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr
  COMPONENTS zephyr_default:boards
  REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

project(image_flasher LANGUAGES NONE)

# Create the runners_yaml_props_target that flash system expects
add_custom_target(runners_yaml_props_target)

# Fetch hex file to flash from sysbuild
zephyr_get(hex_file_to_flash SYSBUILD LOCAL VAR HEX_FILE)
zephyr_get(base_image SYSBUILD LOCAL VAR IMAGE_FLASHER_SPECIFIC_IMAGE)

if(NOT base_image)
  zephyr_get(base_image SYSBUILD GLOBAL VAR IMAGE_FLASHER_DEFAULT_IMAGE)
endif()

# Override the runners.yaml path to use CMAKE_CURRENT_BINARY_DIR/zephyr instead of
# PROJECT_BINARY_DIR, this ensures runners.yaml is generated at <build>/softdevice/zephyr where
# west expects it
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

# Done as a 2-step operation to avoid CI Kconfig compliance issue
set(soc_nrf CONFIG_SOC)
string(APPEND soc_nrf _NRF)

# Copy over Kconfig and dts configuration from the main image
set(base_image_binary_dir ${PROJECT_BINARY_DIR}/../../${base_image}/zephyr)

add_custom_target(${base_image}_kconfig)
import_kconfig(CONFIG_ ${base_image_binary_dir}/.config TARGET ${base_image}_kconfig)
get_target_property(${base_image}_kconfig_keys ${base_image}_kconfig kconfigs)
list(FILTER ${base_image}_kconfig_keys INCLUDE REGEX
  "^${soc_nrf}|^CONFIG_SOC_SERIES|^CONFIG_SOC_FAMILY|^CONFIG_BOARD_"
)
foreach(key ${${base_image}_kconfig_keys})
  get_target_property(value ${base_image}_kconfig ${key})

  if(${value} MATCHES "^((\-?([0-9])+)|(0x([0-9A-Fa-f])+)|y|n)$")
    list(APPEND image_flasher_config "${key}=${value}\n")
  else()
    list(APPEND image_flasher_config "${key}=\"${value}\"\n")
  endif()
endforeach()

file(WRITE ${PROJECT_BINARY_DIR}/.config.new ${image_flasher_config})
zephyr_file_copy(${PROJECT_BINARY_DIR}/.config.new ${PROJECT_BINARY_DIR}/.config
  ONLY_IF_DIFFERENT
)
import_kconfig(CONFIG_ ${PROJECT_BINARY_DIR}/.config)

# Manually include board configuration to enable automatic runners.yaml generation
foreach(dir ${BOARD_DIRECTORIES})
  include(${dir}/board.cmake OPTIONAL)
endforeach()

# Set hex_file property to point to the hex file
set_target_properties(runners_yaml_props_target PROPERTIES
  hex_file "${hex_file_to_flash}"
)

# Include flash support to automatically generate runners.yaml
include(${ZEPHYR_BASE}/cmake/flash/CMakeLists.txt)

# Silent unused variable warnings
set(EXTRA_KCONFIG_TARGETS)
set(FORCED_CONF_FILE)
