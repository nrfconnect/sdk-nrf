#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The upsteram chip_data_model.cmake needs the CHIP_ROOT variable set at include time
# Adding a guard here to make sure it will build properly
if(NOT CHIP_ROOT)
  set(CHIP_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR})
endif()

if(CONFIG_NCS_SAMPLE_MATTER_ZAP_GENERATION_BUILD_TIME)
  include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)
elseif(CONFIG_NCS_SAMPLE_MATTER_ZAP_GENERATION_STATIC)
  include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model_static.cmake)
endif()

function(ncs_configure_data_model)
  string(CONFIGURE "${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH}" zap_file_path)
  cmake_path(GET zap_file_path PARENT_PATH zap_parent_dir)

  cmake_parse_arguments(ARG "" "" "EXTERNAL_CLUSTERS" ${ARGN})

  target_include_directories(matter-data-model
    PUBLIC
    ${zap_parent_dir}
  )

  if(CONFIG_NCS_SAMPLE_MATTER_ZAP_GENERATION_BUILD_TIME)
    # Use the build-time generation implemented in upstream.
    # ZCL_PATH is passed explicitly so generation does not depend on the
    # relative zcl.json path stored inside the .zap file.
    chip_configure_data_model(matter-data-model
      ZAP_FILE ${zap_file_path}
      ZCL_PATH ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/zap-templates/zcl/zcl.json
      EXTERNAL_CLUSTERS ${ARG_EXTERNAL_CLUSTERS}
    )
    # Forward matter-data-model's include directories added by
    # chip_configure_data_model above to 'app'.
    target_include_directories(app PRIVATE
      $<TARGET_PROPERTY:matter-data-model,INCLUDE_DIRECTORIES>
    )
  elseif(CONFIG_NCS_SAMPLE_MATTER_ZAP_GENERATION_STATIC)
    # Use the pre-generated files.
    chip_configure_data_model_static(matter-data-model
      BYPASS_IDL
      GEN_DIR ${zap_parent_dir}/zap-generated
      ZAP_FILE ${zap_file_path}
      EXTERNAL_CLUSTERS ${ARG_EXTERNAL_CLUSTERS}
    )
  else()
    message(WARNING "Unsupported ZAP generation type")
  endif()
endfunction()
