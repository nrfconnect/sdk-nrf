#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

zephyr_library_named(softap_wifi_provision)
zephyr_library_sources(softap_wifi_provision.c)

if (CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATE_REGISTER)
	message(WARNING "SoftAP Wi-Fi provision: Credentials are exposed in non-secure memory. "
			"This should be avoided in production.")

	# Convert imported server certificate from PEM format to HEX format that can be included in
	# C code during provisioning.

	set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/certs)
	zephyr_include_directories(${gen_dir})
	generate_inc_file_for_target(
		app
		${APPLICATION_SOURCE_DIR}/${CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATES_FOLDER}/server_certificate.pem
		${gen_dir}/server_certificate.pem.inc
	)

	generate_inc_file_for_target(
		app
		${APPLICATION_SOURCE_DIR}/${CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATES_FOLDER}/server_private_key.pem
		${gen_dir}/server_private_key.pem.inc
	)

endif()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

set(NANOPB_GENERATE_CPP_STANDALONE OFF)

zephyr_nanopb_sources(softap_wifi_provision
	proto/common.proto
)
