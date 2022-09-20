#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_shell)



set(WIFI_NRF_DRV_BASE ${WEST_TOPDIR}/nrf/drivers/wifi/nrf700x)
if(CONFIG_WPA_SUPP)
set(WPA_SUPP_BASE ${WEST_TOPDIR}/modules/lib/hostap)
endif()

# TODO: MbedTLS Remove this workaround
target_include_directories(mbedcrypto_common
	INTERFACE
	${CMAKE_SOURCE_DIR}
)

if(CONFIG_WIFI_PROVISIONING_PROTO_BUILD_TIME_GENERATION)

set(PROTOC_OPTIONS "-I${CMAKE_SOURCE_DIR}/common/proto")
set(NANOPB_OPTIONS "-I${CMAKE_SOURCE_DIR}")

nanopb_generate_cpp(proto_sources proto_headers RELPATH .
	common/proto/common.proto
	common/proto/version.proto
	common/proto/result.proto
	common/proto/request.proto
	common/proto/response.proto
)
# add include path for generated pb.c
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})
# add include path for source files
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR}/common/proto)

else()

#add include path for pb.h
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/proto-c)
#add pb.c files
file(GLOB proto_sources ${CMAKE_CURRENT_SOURCE_DIR}/common/proto-c/*.c)

endif()

target_sources(app PRIVATE src/main.c
			common/wifi_prov_handler.c
			common/wifi_prov_ble.c
			common/wifi_config.c
			${proto_sources}
			)

zephyr_library_include_directories(inc)
