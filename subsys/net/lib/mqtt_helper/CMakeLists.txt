#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(mqtt_helper.c)

if (CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES)
	message(WARNING "Credentials are exposed in non-secure memory. This should be avoided in production.")

	# Define the directory where certificate include files will be stored.
	set(app_certs_binary_dir ${APPLICATION_BINARY_DIR}/certs)
	zephyr_include_directories(cert)

	# Function to process a certificate file and create a corresponding .inc file and compiler definition
	# used in certs/mqtt-certs.h to assign C variables that are used in mqtt_helper.c.
	function(process_certificate definition_name file_name)
		set(cert_file ${APPLICATION_SOURCE_DIR}/${CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER}/${file_name})
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
	process_certificate("MQTT_HELPER_CA_CERT" "ca-cert.pem")
	process_certificate("MQTT_HELPER_CLIENT_CERT" "client-cert.pem")
	process_certificate("MQTT_HELPER_PRIVATE_KEY" "private-key.pem")
	process_certificate("MQTT_HELPER_CA_CERT_2" "ca-cert-2.pem")
	process_certificate("MQTT_HELPER_CLIENT_CERT_2" "client-cert-2.pem")
	process_certificate("MQTT_HELPER_PRIVATE_KEY_2" "private-key-2.pem")

endif()
