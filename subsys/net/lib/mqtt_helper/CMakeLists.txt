#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(mqtt_helper.c)
zephyr_include_directories_ifdef(CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES cert)

if (CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES)
	message(WARNING "Credentials are exposed in non-secure memory. This should be avoided in production.")
endif()
