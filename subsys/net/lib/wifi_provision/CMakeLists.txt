#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

zephyr_library_named(wifi_provision)
zephyr_library_sources(wifi_provision.c)

message(WARNING "Wi-Fi provision: Credentials are exposed in non-secure memory. This should be avoided in production.")

# Convert imported server certificate from PEM format to HEX format that can be included in C code during provisioning.
set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/certs)
zephyr_include_directories(${gen_dir})
generate_inc_file_for_target(
	app
	certs/server_certificate.pem
	${gen_dir}/server_certificate.pem.inc
)

generate_inc_file_for_target(
	app
	certs/server_private_key.pem
	${gen_dir}/server_private_key.pem.inc
)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

set(NANOPB_GENERATE_CPP_STANDALONE OFF)

zephyr_nanopb_sources(wifi_provision
	proto/common.proto
)
