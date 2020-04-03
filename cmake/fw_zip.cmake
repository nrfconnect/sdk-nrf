
#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(generate_dfu_zip)
  set(oneValueArgs OUTPUT TYPE)
  set(multiValueArgs DEPENDENCIES BIN_FILES SCRIPT_PARAMS)
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
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/bootloader/generate_zip.py
    --bin-files ${GENZIP_BIN_FILES}
    --output ${GENZIP_OUTPUT}
    ${GENZIP_SCRIPT_PARAMS}
    "type=${GENZIP_TYPE}"
    "board=${CONFIG_BOARD}"
    "soc=${CONFIG_SOC}"
    DEPENDS
    ${GENZIP_DEPENDENCIES}
    OUTPUT ${GENZIP_OUTPUT}
    )

  list(GET GENZIP_DEPENDENCIES 0 first_dep)
  string(RANDOM rnd)

  add_custom_target(
    genzip_${first_dep}_${rnd}
    ALL
    DEPENDS
    ${GENZIP_OUTPUT}
    )
endfunction()
