#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(EI_URI ${CONFIG_EDGE_IMPULSE_URI})

if(${EI_URI} MATCHES "^[a-z]+://")
   message(FATAL_ERROR "Unit test must generate the zip file with mocked Edge Impulse library"
                       " on local hard drive.")
endif()

string(CONFIGURE ${EI_URI} EI_URI)
if(NOT IS_ABSOLUTE ${EI_URI})
  set(EI_URI ${APPLICATION_SOURCE_DIR}/${EI_URI})
endif()

add_custom_target(create_zip COMMAND
    ${CMAKE_COMMAND} -E tar "cf"
    ${EI_URI} --format=zip
    "./"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/edge_impulse_zip/")

# Ensure that zip will be packed before Edge Impulse archive is unzipped.
add_dependencies(edge_impulse_project create_zip)

project("Edge Impulse test")

target_sources(app PRIVATE src/main.cpp)
# Test uses ei_test_params.h file from edge_impuse_zip directory to verify if
# ei_wrapper properly forwards the data between application and EI library.
target_include_directories(app PRIVATE src/edge_impulse_zip/)
