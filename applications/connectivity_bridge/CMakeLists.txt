#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Git)
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Generate a C header file with the readme text as a string variable
set(OUTPUT_README_HEADER_FILE "${CMAKE_BINARY_DIR}/generated/readme/readme.h")
set(INPUT_README_FILE ${CMAKE_SOURCE_DIR}/boards/${NORMALIZED_BOARD_TARGET}_readme.txt)

if(NOT EXISTS ${INPUT_README_FILE})
  message(WARNING "${INPUT_README_FILE} does not exist")
  set(INPUT_README_FILE ${CMAKE_SOURCE_DIR}/README_TEMPLATE.txt)
endif()

file(READ ${INPUT_README_FILE} README_CONTENT)
if(GIT_FOUND)
  execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
  string(APPEND README_CONTENT "\nFirmware version: ${GIT_HASH} Build date: ${CURRENT_DATE}")
endif()
string(REPLACE "\"" "\\\"" README_CONTENT "${README_CONTENT}")
string(REPLACE "\n" "\\n\"\n\"" README_CONTENT "${README_CONTENT}")
file(WRITE ${OUTPUT_README_HEADER_FILE} "\"${README_CONTENT}\"")


# Include application events and disk files
zephyr_library_include_directories(
  src/events
  ${CMAKE_BINARY_DIR}/generated/readme
  )

# Application sources
add_subdirectory(src/disk)
add_subdirectory(src/events)
add_subdirectory(src/modules)
