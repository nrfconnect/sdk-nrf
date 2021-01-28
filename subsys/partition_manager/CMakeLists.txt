#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(preprocess_pm_yml in_file out_file)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    -I${ZEPHYR_BINARY_DIR}/include/generated
    ${NOSYSDEF_CFLAG}
    -P
    -E ${in_file}
    -o ${out_file}
    WORKING_DIRECTORY ${ZEPHYR_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  if (DEFINED IMAGE_NAME)
    # Share location of original source file so that the parent image can add it
    # to the CMAKE_CONFIGURE_DEPENDS list.
    share("list(APPEND ${IMAGE_NAME}PM_YML_DEP_FILES ${in_file})")

    # Share location of preprocessed pm.yml file so that the parent image can
    # use it as source for partition manager configuration.
    share("list(APPEND ${IMAGE_NAME}PM_YML_FILES ${out_file})")
  endif()

  # Re-configure (Re-execute all CMakeLists.txt code) when original
  # (not preprocessed) configuration file changes.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${in_file})

endfunction()


# Add all pm.yml files for subsystems
# Store the preprocessed output in the binary dir of the subsystems
# to avoid overwriting pm.yml files.
if (CONFIG_SETTINGS_FCB OR CONFIG_SETTINGS_NVS)
  ncs_add_partition_manager_config(pm.yml.settings)
endif()

if (CONFIG_FILE_SYSTEM)
  ncs_add_partition_manager_config(pm.yml.file_system)
endif()

if (CONFIG_ZIGBEE)
  ncs_add_partition_manager_config(pm.yml.zboss)
endif()

if (CONFIG_NVS AND NOT CONFIG_SETTINGS_NVS)
  ncs_add_partition_manager_config(pm.yml.nvs)
endif()

if (CONFIG_NRF_MODEM_LIB)
  ncs_add_partition_manager_config(pm.yml.libmodem)
endif()

if (CONFIG_BT_RPMSG_NRF53)
  ncs_add_partition_manager_config(pm.yml.bt_rpmsg_nrf53)
endif()

if (CONFIG_NRF53_UPGRADE_NETWORK_CORE)
  ncs_add_partition_manager_config(pm.yml.pcd)
endif()

if (CONFIG_BUILD_WITH_TFM)
  ncs_add_partition_manager_config(pm.yml.tfm)
endif()

if (CONFIG_TRUSTED_EXECUTION_NONSECURE OR CONFIG_TRUSTED_EXECUTION_SECURE)
  ncs_add_partition_manager_config(pm.yml.trustzone)
endif()

# We are using partition manager if we are a child image or if we are
# the root image and the 'partition_manager' target exists.
set(using_partition_manager
  $<OR:$<BOOL:${IMAGE_NAME}>,$<TARGET_EXISTS:partition_manager>>
  )
zephyr_compile_definitions(
  USE_PARTITION_MANAGER=${using_partition_manager}
  )

# TODO: check how this patch got lost and if more are missing
set_property(GLOBAL APPEND PROPERTY
  PROPERTY_LINKER_SCRIPT_DEFINES
  -DUSE_PARTITION_MANAGER=${using_partition_manager}
  )

if((EXISTS ${CMAKE_SOURCE_DIR}/pm.yml) AND IMAGE_NAME)
  # Only preprocess pm.yml when being built as sub image.

  preprocess_pm_yml(
    ${CMAKE_SOURCE_DIR}/pm.yml
    ${ZEPHYR_BINARY_DIR}/include/generated/pm.yml
    )
endif()

get_property(PM_SUBSYS_PATHS GLOBAL PROPERTY PM_SUBSYS_PATHS)
get_property(PM_SUBSYS_OUTPUT_PATHS GLOBAL PROPERTY PM_SUBSYS_OUTPUT_PATHS)

# Check for partition manager configurations defined by subsystems
# This is a list of absolute paths to these pm.yml files.
if (PM_SUBSYS_PATHS)
  # Each entry in the list has a corresponding entry with the output
  # path in the build directory for the pm.yml file.
  foreach (pm_yml_path ${PM_SUBSYS_PATHS})
    list(GET PM_SUBSYS_OUTPUT_PATHS 0 output_pm_yml_path)
    list(REMOVE_AT PM_SUBSYS_OUTPUT_PATHS 0)

    preprocess_pm_yml(
      ${pm_yml_path}
      ${output_pm_yml_path}
      )
    set_property(
      GLOBAL APPEND PROPERTY
      PM_SUBSYS_PREPROCESSED
      ${output_pm_yml_path}
      )
  endforeach()
endif()
