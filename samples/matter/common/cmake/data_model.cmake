#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

string(CONFIGURE "${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH}" zap_file_path)
cmake_path(GET zap_file_path PARENT_PATH zap_parent_dir)

# If we have a new app zap dir assigned we need to include zap parent dir to app target as well.
if(CONFIG_CHIP_APP_ZAP_DIR)
  string(CONFIGURE "${CONFIG_CHIP_APP_ZAP_DIR}" app_zap_dir)

  # Override zap-generated directory.
  get_filename_component(CHIP_APP_ZAP_DIR ${app_zap_dir} REALPATH CACHE)
else()
  string(CONFIGURE "${zap_parent_dir}/zap-generated" app_zap_dir)
endif()

include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

function(ncs_configure_data_model)
  cmake_parse_arguments(ARG "" "" "EXTERNAL_CLUSTERS" ${ARGN})

  target_include_directories(matter-data-model
    PUBLIC
    ${zap_parent_dir}
  )

  chip_configure_data_model(matter-data-model
    BYPASS_IDL
    GEN_DIR ${app_zap_dir}
    ZAP_FILE ${zap_file_path}
    EXTERNAL_CLUSTERS ${ARG_EXTERNAL_CLUSTERS}
  )
endfunction()
