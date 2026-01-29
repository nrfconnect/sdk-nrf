#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Usage:
#   add_image_flasher(NAME <name> HEX_FILE <hex_file>)
#
# Adds a sysbuild image which will flash the hex file when `west flash` is invoked.
function(add_image_flasher)
  set(single_args NAME HEX_FILE)
  cmake_parse_arguments(args "" "${single_args}" "" ${ARGN})

  if(NOT args_NAME)
    message(FATAL_ERROR "Argument NAME is required")
  elseif(NOT args_HEX_FILE)
    message(FATAL_ERROR "Argument HEX_FILE is required")
  elseif(NOT IS_ABSOLUTE "${args_HEX_FILE}")
    message(FATAL_ERROR "Argument HEX_FILE must be an absolute path")
  elseif(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown arguments specified: ${args_UNPARSED_ARGUMENTS}")
  endif()

  set(${args_NAME}_HEX_FILE "${args_HEX_FILE}" CACHE FILEPATH "Hex file to flash" FORCE)
  set(IMAGE_FLASHER_DEFAULT_IMAGE ${DEFAULT_IMAGE} CACHE STRING "Default image" FORCE)

  ExternalZephyrProject_Add(
    APPLICATION ${args_NAME}
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/applications/image_flasher
  )

  # Must be configured after the default image as the default image Kconfig and dts files are
  # copied.
  sysbuild_add_dependencies(CONFIGURE ${args_NAME} ${DEFAULT_IMAGE})
endfunction()
