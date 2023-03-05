#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# LTE boards
target_sources_ifdef(CONFIG_NRF_MODEM_LIB
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/network_lte.c)

# Wifi boards
target_sources_ifdef(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/network_wifi.c)

# Emulator boards
target_sources_ifdef(CONFIG_BOARD_NATIVE_POSIX
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/network_emulation.c)

# Add credentials provision library if the Modem key Management API is enabled.
# The library provisions credentials placed in the src/transport/credentials/ folder to
# the nRF9160 modem.
add_subdirectory_ifdef(CONFIG_MODEM_KEY_MGMT credentials_provision)

# Make the folder that contains the CA certificate for "test.mosquitto.org" global so that it can
# be located by the MQTT helper library / local credential provision library.
#
# The MQTT helper library provisions the certificate
# to the Mbed TLS stack for Native Posix and nRF7002 DK builds.
#
# The credential provision library provisions the certificate
# to the modem's network stack for nRF9160 builds.
#
# CONFIG_MODEM_KEY_MGMT and CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES are mutually exclusive.
#
zephyr_include_directories_ifdef(CONFIG_MODEM_KEY_MGMT credentials)
zephyr_include_directories_ifdef(CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES credentials)
