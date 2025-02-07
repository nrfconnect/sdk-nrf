#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(script_args ${CMAKE_BINARY_DIR})

if(SB_CONFIG_COMP_DATA_LAYOUT_SINGLE)
  list(PREPEND script_args --single)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_COMP_DATA_LAYOUT_SINGLE y)
else()
  list(PREPEND script_args --multiple)
  if(SB_CONFIG_COMP_DATA_LAYOUT_MULTIPLE)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_COMP_DATA_LAYOUT_MULTIPLE y)
  elseif(SB_CONFIG_COMP_DATA_LAYOUT_ARRAY)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_COMP_DATA_LAYOUT_ARRAY y)
  endif()
endif()

if(SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA_FWID_CUSTOM)
  list(APPEND script_args --fwid-custom)
elseif(SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA_FWID_MCUBOOT_VERSION)
  list(APPEND script_args --fwid-mcuboot-version)
else()
  list(APPEND script_args --no-fwid)
endif()

add_custom_target(verify_metadata ALL
  ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/verify_metadata.py ${script_args}
  DEPENDS parse_mesh_metadata
)
