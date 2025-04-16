#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

function(ncs_configure_data_model)
  cmake_parse_arguments(ARG "" "ZAP_FILE" "EXTERNAL_CLUSTERS" ${ARGN})

  if(NOT ARG_ZAP_FILE)
    message(FATAL_ERROR "ZAP_FILE argument is required")
  endif()

  target_include_directories(matter-data-model
    PUBLIC
    ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}
  )

  chip_configure_data_model(matter-data-model
    BYPASS_IDL
    GEN_DIR ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/zap-generated
    ZAP_FILE ${ARG_ZAP_FILE}
    EXTERNAL_CLUSTERS ${ARG_EXTERNAL_CLUSTERS}
  )
endfunction()
