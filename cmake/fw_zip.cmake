#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(generate_dfu_zip)
  set(oneValueArgs OUTPUT TYPE TARGET)
  set(multiValueArgs BIN_FILES SCRIPT_PARAMS)
  cmake_parse_arguments(GENZIP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT(
    GENZIP_BIN_FILES AND
    GENZIP_SCRIPT_PARAMS AND
    GENZIP_OUTPUT AND
    GENZIP_TYPE AND
    GENZIP_TARGET
    ))
    message(FATAL_ERROR "Missing required param")
  endif()

  add_custom_command(
    OUTPUT ${GENZIP_OUTPUT}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/bootloader/generate_zip.py
    --bin-files ${GENZIP_BIN_FILES}
    --output ${GENZIP_OUTPUT}
    ${GENZIP_SCRIPT_PARAMS}
    "type=${GENZIP_TYPE}"
    "board=${CONFIG_BOARD}"
    "soc=${CONFIG_SOC}"
    DEPENDS ${GENZIP_TARGET}
    )

  get_filename_component(TARGET_NAME ${GENZIP_OUTPUT} NAME)
  string(REPLACE "." "_" TARGET_NAME ${TARGET_NAME})

  add_custom_target(
    ${TARGET_NAME}
    ALL
    DEPENDS ${GENZIP_OUTPUT}
    )

endfunction()
