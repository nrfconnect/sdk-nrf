#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY                ${COMMIT_PATH}
    OUTPUT_VARIABLE                  ${COMMIT_TYPE}_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )

  if(return_code)
    message(STATUS "git rev-parse failed: ${stderr}")
  elseif(NOT "${stderr}" STREQUAL "")
    message(STATUS "git rev-parse warned: ${stderr}")
  endif()

  # Limit to 12 characters
  string(SUBSTRING "${${COMMIT_TYPE}_COMMIT}" 0 12 ${COMMIT_TYPE}_COMMIT)
endif()

file(READ ${NRF_DIR}/cmake/commit.h.in commit_content)
string(CONFIGURE "${commit_content}" commit_content)
string(CONFIGURE "${commit_content}" commit_content)

if(EXISTS ${OUT_FILE})
  file(READ ${OUT_FILE} current_contents)
else()
  set(current_contents "")
endif()

if(NOT "${current_contents}" STREQUAL "${commit_content}")
  file(WRITE ${OUT_FILE} "${commit_content}")
endif()
