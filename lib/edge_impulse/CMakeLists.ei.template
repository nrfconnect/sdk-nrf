#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR "arm")

project(edge_impulse)
enable_language(ASM C CXX)

if(EI_COMPILE_DEFINITIONS)
  add_compile_definitions(${EI_COMPILE_DEFINITIONS})
endif()

if(EI_INCLUDE_DIRECTORIES)
  include_directories(${EI_INCLUDE_DIRECTORIES})
endif()

if(EI_SYSTEM_INCLUDE_DIRECTORIES)
  include_directories(SYSTEM ${EI_SYSTEM_INCLUDE_DIRECTORIES})
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../compile_options.CXX.cmake OPTIONAL)
include(${CMAKE_CURRENT_LIST_DIR}/../../compile_options.C.cmake OPTIONAL)

set(EI_COMPILE_OPTIONS ${EI_C_COMPILE_OPTIONS})
list(REMOVE_ITEM EI_C_COMPILE_OPTIONS ${EI_CXX_COMPILE_OPTIONS} "")
list(REMOVE_ITEM EI_COMPILE_OPTIONS ${EI_C_COMPILE_OPTIONS})
list(REMOVE_ITEM EI_CXX_COMPILE_OPTIONS ${EI_COMPILE_OPTIONS} "")

# Skip warnings for the source files
set(SKIP_WARNINGS -Wall)
list(REMOVE_ITEM EI_COMPILE_OPTIONS ${SKIP_WARNINGS})
list(REMOVE_ITEM EI_C_COMPILE_OPTIONS ${SKIP_WARNINGS})
list(REMOVE_ITEM EI_CXX_COMPILE_OPTIONS ${SKIP_WARNINGS})

add_compile_options(${EI_COMPILE_OPTIONS})
foreach(option ${EI_C_COMPILE_OPTIONS})
  add_compile_options($<$<COMPILE_LANGUAGE:C>:${option}>)
endforeach()

foreach(option ${EI_CXX_COMPILE_OPTIONS})
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${option}>)
endforeach()

if(NOT TARGET app)
  add_library(app STATIC)
endif()

if(EI_LIBRARY_NAME)
  set_property(TARGET app PROPERTY OUTPUT_NAME ${EI_LIBRARY_NAME})
endif()

include(${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/cmake/utils.cmake)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/cmake/zephyr)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/tflite-model
    ${CMAKE_CURRENT_LIST_DIR}/model-parameters
)

# Find model source files
RECURSIVE_FIND_FILE(MODEL_FILES "${CMAKE_CURRENT_LIST_DIR}/tflite-model" "*.cpp")
list(APPEND SOURCE_FILES ${MODEL_FILES})

# Add all sources to the project
target_sources(app PRIVATE ${SOURCE_FILES})
