#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_include_directories_ifdef(CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES cert)
zephyr_library_sources(mqtt_helper.c)
