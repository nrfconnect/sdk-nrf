#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_ble_provisioning)

# Add path to .proto files so that one can import another
set(PROTOC_OPTIONS "-I${CMAKE_CURRENT_SOURCE_DIR}/common/proto")
# Note that here, we're adding CMAKE_SOURCE_DIR to the include path for nanopb.
# This is needed because the below call to nanopb_generate_cpp() is using
# 'RELPATH .'
set(NANOPB_OPTIONS "-I${CMAKE_SOURCE_DIR}")
# Add .proto files here
nanopb_generate_cpp(proto_sources proto_headers RELPATH .
  common/proto/common.proto
  common/proto/version.proto
  common/proto/result.proto
  common/proto/request.proto
  common/proto/response.proto
)
# Add include path to generated .pb.h header files
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_sources(app PRIVATE src/main.c
                           common/wifi_prov_handler.c
                           common/wifi_prov_ble.c
                           ${proto_sources}
)

zephyr_library_include_directories(inc)
