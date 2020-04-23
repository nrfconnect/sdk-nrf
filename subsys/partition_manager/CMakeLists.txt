#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

endfunction()

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

# Check for partition manager configurations defined by subsystems
# This is a list of absolute paths to these pm.yml files.
get_property(pm_yml_paths GLOBAL PROPERTY PM_SUBSYS_PATH)
if (pm_yml_paths)
  # Each entry in the list has a corresponding entry with the output
  # path in the build directory for the pm.yml file.
  get_property(output_pm_yml_paths GLOBAL PROPERTY PM_SUBSYS_OUTPUT_PATH)
  foreach (pm_yml_path ${pm_yml_paths})
    list(GET output_pm_yml_paths 0 output_pm_yml_path)
    list(REMOVE_AT output_pm_yml_paths 0)

    preprocess_pm_yml(
      ${pm_yml_path}
      ${output_pm_yml_path}/pm.yml
      )
    set_property(
      GLOBAL APPEND PROPERTY
      PM_SUBSYS_PREPROCESSED
      ${output_pm_yml_path}/pm.yml
      )
  endforeach()
endif()
