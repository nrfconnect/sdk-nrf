#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
	azure_fota.c
)

find_package(Git QUIET)
if(NOT AZURE_FOTA_APP_VERSION AND GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe
    WORKING_DIRECTORY                ${ZEPHYR_NRF_MODULE_DIR}
    OUTPUT_VARIABLE                  AZURE_FOTA_APP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )
  if(return_code)
    message(STATUS "git describe failed: ${stderr};
    AZURE_FOTA_APP_VERSION is left undefined")
  elseif(CMAKE_VERBOSE_MAKEFILE)
    message(STATUS "git describe stderr: ${stderr}")
  endif()
endif()

if(AZURE_FOTA_APP_VERSION)
  zephyr_compile_definitions(
    AZURE_FOTA_APP_VERSION=${AZURE_FOTA_APP_VERSION}
  )
endif()

if(NOT PROJECT_NAME)
  zephyr_compile_definitions(
    PROJECT_NAME=${CMAKE_PROJECT_NAME}
  )
endif()
