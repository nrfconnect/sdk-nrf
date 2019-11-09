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
    -I${PROJECT_BINARY_DIR}/include/generated
    ${NOSYSDEF_CFLAG}
    -P
    -E ${in_file}
    -o ${out_file}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

endfunction()

zephyr_compile_definitions(
  USE_PARTITION_MANAGER=$<TARGET_EXISTS:partition_manager>
  )

if((EXISTS ${CMAKE_SOURCE_DIR}/pm.yml) AND GENERATE_PARTITION_MANAGER_ENTRY)
  # Only preprocess pm.yml when being built as sub image.

  preprocess_pm_yml(
    ${CMAKE_SOURCE_DIR}/pm.yml
    ${PROJECT_BINARY_DIR}/include/generated/pm.yml
    )
endif()

get_property(img_pm_subsys GLOBAL PROPERTY ${IMAGE}PM_SUBSYS)
if (img_pm_subsys)
  foreach (subsys_pm_yml ${img_pm_subsys})
    file(RELATIVE_PATH rel_path_to_yml ${ZEPHYR_BASE} ${subsys_pm_yml})
    set(subsys_pm_preprocessed
      ${PROJECT_BINARY_DIR}/${rel_path_to_yml})
    preprocess_pm_yml(
      ${subsys_pm_yml}
      ${subsys_pm_preprocessed}
      )
  set_property(
    GLOBAL APPEND PROPERTY
    PM_SUBSYS_PREPROCESSED
      ${subsys_pm_preprocessed}
    )
  endforeach()
endif()
