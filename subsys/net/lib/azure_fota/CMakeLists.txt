#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(
	azure_fota.c
)

find_package(Git QUIET)
if(NOT APP_VERSION AND GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe
    WORKING_DIRECTORY                ${NRF_DIR}
    OUTPUT_VARIABLE                  APP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )
  if(return_code)
    message(STATUS "git describe failed: ${stderr};
    APP_VERSION is left undefined")
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
