#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

function(ncs_configure_data_model)
  # Create a path to the zap file based on Kconfigs and cmake sample source dir
  cmake_path(SET ZAP_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_NAME})

  cmake_parse_arguments(ARG "" "" "EXTERNAL_CLUSTERS" ${ARGN})

  target_include_directories(matter-data-model
    PUBLIC
    ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}
  )

  chip_configure_data_model(matter-data-model
    BYPASS_IDL
    GEN_DIR ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/zap-generated
    ZAP_FILE ${ZAP_FILE_PATH}
    EXTERNAL_CLUSTERS ${ARG_EXTERNAL_CLUSTERS}
  )
endfunction()
