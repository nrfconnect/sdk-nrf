#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

find_package(Python3 REQUIRED)

function(mesh_dfu_metadata)
    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/dfu_application.zip_ble_mesh_metadata.json
        COMMAND
        ${PYTHON_EXECUTABLE}
        ${ZEPHYR_NRF_MODULE_DIR}/scripts/bluetooth/mesh/mesh_dfu_metadata.py
        ${PROJECT_BINARY_DIR}
        DEPENDS
        ${PROJECT_BINARY_DIR}/dfu_application.zip
    )

    add_custom_target(
        parse_mesh_metadata
        ALL
        DEPENDS ${PROJECT_BINARY_DIR}/dfu_application.zip_ble_mesh_metadata.json
    )
endfunction()
