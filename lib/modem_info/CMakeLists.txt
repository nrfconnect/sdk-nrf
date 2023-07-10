#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(modem_info.c)
zephyr_library_sources(modem_info_params.c)

find_package(Git QUIET)
if(NOT APP_VERSION AND GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=12
    WORKING_DIRECTORY                ${ZEPHYR_NRF_MODULE_DIR}
    OUTPUT_VARIABLE                  APP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )
  if(return_code)
    message(STATUS "git describe failed: ${stderr}; ${KERNEL_VERSION_STRING} will be used instead")
  elseif(CMAKE_VERBOSE_MAKEFILE)
    message(STATUS "git describe stderr: ${stderr}")
  endif()
endif()

if(APP_VERSION)
  zephyr_compile_definitions(
    APP_VERSION=${APP_VERSION}
  )
endif()

if(NOT PROJECT_NAME)
  zephyr_compile_definitions(
    PROJECT_NAME=${CMAKE_PROJECT_NAME}
  )
endif()
