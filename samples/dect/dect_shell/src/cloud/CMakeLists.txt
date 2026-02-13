#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources_ifdef(CONFIG_DESH_CLOUD_MQTT app PRIVATE cloud_mqtt_shell.c)
target_sources_ifdef(CONFIG_DESH_CLOUD_COAP app PRIVATE cloud_coap_shell.c)
