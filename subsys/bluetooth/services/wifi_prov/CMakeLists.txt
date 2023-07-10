#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()

set(PROTOC_OPTIONS "-I${CMAKE_CURRENT_SOURCE_DIR}")
set(NANOPB_OPTIONS "nanopb_BUILD_RUNTIME OFF")
set(CMAKE_MODULE_PATH ${ZEPHYR_NANOPB_MODULE_DIR}/extra)
find_package(Nanopb REQUIRED)
set(NANOPB_GENERATE_CPP_STANDALONE FALSE)
nanopb_generate_cpp(proto_sources proto_headers
	proto/common.proto
	proto/version.proto
	proto/result.proto
	proto/request.proto
	proto/response.proto
)

# Add include path to generated .pb.h header files
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})

zephyr_library_sources(
	wifi_prov_ble.c
	wifi_prov_handler.c
	${proto_sources})
