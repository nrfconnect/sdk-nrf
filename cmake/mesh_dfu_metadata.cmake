#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

find_package(Python3 REQUIRED)

function(mesh_dfu_metadata)
  if(SYSBUILD)
    set(metadata_dir ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr)
  else()
    set(metadata_dir ${PROJECT_BINARY_DIR})
  endif()

  set(metadata_depends ${CMAKE_BINARY_DIR}/dfu_application.zip)

  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_application.zip_ble_mesh_metadata.json
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bluetooth/mesh/mesh_dfu_metadata.py
    --bin-path ${metadata_dir}
    DEPENDS ${metadata_depends}
  )

  add_custom_target(
    parse_mesh_metadata
    ALL
    DEPENDS ${PROJECT_BINARY_DIR}/dfu_application.zip_ble_mesh_metadata.json
  )

  add_custom_target(
    # Prints already generated metadata
    ble_mesh_dfu_metadata
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bluetooth/mesh/mesh_dfu_metadata.py
    --bin-path ${PROJECT_BINARY_DIR}
    --print-metadata
    COMMAND_EXPAND_LISTS
  )
endfunction()
