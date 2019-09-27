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
set_property(GLOBAL APPEND PROPERTY
  ${IMAGE}PROPERTY_LINKER_SCRIPT_DEFINES
  -DUSE_PARTITION_MANAGER=$<TARGET_EXISTS:partition_manager>
  )

if((EXISTS ${APPLICATION_SOURCE_DIR}/pm.yml) AND IMAGE)
  # Only preprocess pm.yml when being built as sub image.

  preprocess_pm_yml(
    ${APPLICATION_SOURCE_DIR}/pm.yml
    ${PROJECT_BINARY_DIR}/include/generated/pm.yml
    )

  set_property(
    GLOBAL APPEND PROPERTY
    PM_IMAGES
    ${IMAGE}
    )
endif()
