#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA)
  add_custom_target(ble_mesh_print_dfu_fwid ALL
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/dfu/common/script/ble_mesh_dfu_fwid.py
  --bin-path ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr
  DEPENDS parse_mesh_metadata
  )
endif()
