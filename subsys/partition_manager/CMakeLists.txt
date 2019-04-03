#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_compile_definitions(
  USE_PARTITION_MANAGER=$<TARGET_EXISTS:partition_manager>
  )
set_property(GLOBAL APPEND PROPERTY
  ${IMAGE}PROPERTY_LINKER_SCRIPT_DEFINES
  -DUSE_PARTITION_MANAGER=$<TARGET_EXISTS:partition_manager>
  )

if((EXISTS ${APPLICATION_SOURCE_DIR}/pm.yml) AND IMAGE)
  # Only run partition manager when being built as sub image.

  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    -I${PROJECT_BINARY_DIR}/include/generated
    ${NOSYSDEF_CFLAG}
    -P
    -E ${APPLICATION_SOURCE_DIR}/pm.yml
    -o ${PROJECT_BINARY_DIR}/include/generated/pm.yml
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()
  string(LENGTH ${IMAGE} len)
  MATH(EXPR len "${len}-1")
  string(SUBSTRING ${IMAGE} 0 ${len} image_name)
  set_property(
    GLOBAL APPEND PROPERTY
    PARTITION_MANAGER_CONFIG_FILES
    "${image_name}:${PROJECT_BINARY_DIR}/include/generated/pm.yml:${PROJECT_BINARY_DIR}:${PROJECT_BINARY_DIR}/include/generated"
    )
  set_property(
    GLOBAL APPEND PROPERTY
    PM_HEX_FILES_TO_MERGE
    ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
    )
  set_property(
    GLOBAL APPEND PROPERTY
    PM_HEX_FILES_TO_MERGE_TARGETS
    ${IMAGE}kernel_elf
    )
endif()
