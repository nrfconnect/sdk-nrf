# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

find_program(NRF_REGTOOL nrf-regtool REQUIRED)

execute_process(
  COMMAND
  ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
  ${NRF_REGTOOL}
  uicr-compile
  --edt-pickle-file ${EDT_PICKLE}
  --product-name ${CONFIG_SOC}
  --output-file ${PROJECT_BINARY_DIR}/uicr_legacy.hex
  WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
  COMMAND_ERROR_IS_FATAL ANY
  )

execute_process(
  COMMAND
  ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
  ${NRF_REGTOOL}
  uicr-migrate
  --edt-pickle-file ${EDT_PICKLE}
  --uicr-hex-file ${PROJECT_BINARY_DIR}/uicr_legacy.hex
  --output-periphconf-file ${PROJECT_BINARY_DIR}/periphconf_migrated.c
  WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
  COMMAND_ERROR_IS_FATAL ANY
  )
message(STATUS "Generated periphconf_migrated.c: ${PROJECT_BINARY_DIR}/periphconf_migrated.c")

zephyr_sources(${PROJECT_BINARY_DIR}/periphconf_migrated.c)
