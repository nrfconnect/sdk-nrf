#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(http_server)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/credentials_provision.c)
target_include_directories(app PRIVATE src)

if (CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	message(WARNING "Credentials are exposed in non-secure memory. This should be avoided in production.")

	set(app_certs_binary_dir ${APPLICATION_BINARY_DIR}/credentials)

	# Function to process a certificate file and create a corresponding .inc file and compiler definition
	# used to assign C variables that are used in main.c.
	function(process_certificate definition_name file_name)
		set(cert_file ${APPLICATION_SOURCE_DIR}/credentials/${file_name})
		if(EXISTS ${cert_file})
			message(STATUS "${file_name} found")

			get_filename_component(file_base_name ${file_name} NAME_WE)
			set(inc_file_name ${file_base_name}.inc)

			set(inc_file_path ${app_certs_binary_dir}/${inc_file_name})
			generate_inc_file_for_target(app ${cert_file} ${inc_file_path})

			# Define a compiler macro with the path to the generated .inc file,
			# allowing it to be included in the source code.
			add_definitions(-D${definition_name}="${inc_file_path}")
		endif()
	endfunction()

	# Process each certificate file by generating a .inc file and defining a corresponding macro.
	process_certificate("HTTP_SERVER_CA_CERT" "server_certificate.pem")
	process_certificate("HTTP_SERVER_PRIVATE_KEY" "server_private_key.pem")

endif()
