#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/transport.c)

# Add Client ID helper library
add_subdirectory(client_id)

# Add credentials provision library if the Modem key Management API is enabled.
# The library provisions credentials placed in the src/transport/credentials/ folder to
# the nRF91 modem.
add_subdirectory_ifdef(CONFIG_MODEM_KEY_MGMT credentials_provision)

if(CONFIG_MODEM_KEY_MGMT)
	# Generate include files from pem files
	set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/certs)
	zephyr_include_directories(${gen_dir})
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/credentials/ca-cert.pem)
		generate_inc_file_for_target(app credentials/ca-cert.pem ${gen_dir}/ca-cert.pem)
	endif()
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/credentials/client-cert.pem)
		generate_inc_file_for_target(app credentials/client-cert.pem ${gen_dir}/client-cert.pem)
	endif()
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/credentials/private-key.pem)
		generate_inc_file_for_target(app credentials/private-key.pem ${gen_dir}/private-key.pem)
	endif()
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/credentials/ca-cert-2.pem)
		generate_inc_file_for_target(app credentials/ca-cert-2.pem ${gen_dir}/ca-cert-2.pem)
	endif()
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/credentials/client-cert-2.pem)
		generate_inc_file_for_target(app credentials/client-cert-2.pem ${gen_dir}/client-cert-2.pem)
	endif()
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/credentials/private-key-2.pem)
		generate_inc_file_for_target(app credentials/private-key-2.pem ${gen_dir}/private-key-2.pem)
	endif()
endif()
