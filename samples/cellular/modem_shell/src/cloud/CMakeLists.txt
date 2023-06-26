#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources_ifdef(CONFIG_MOSH_CLOUD_MQTT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_mqtt_shell.c)
target_sources_ifdef(CONFIG_MOSH_CLOUD_REST app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_rest_shell.c)
target_sources_ifdef(CONFIG_MOSH_CLOUD_COAP app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_coap_shell.c)
target_sources_ifdef(CONFIG_MOSH_CLOUD_LWM2M app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_lwm2m_shell.c)
target_sources_ifdef(CONFIG_MOSH_CLOUD_LWM2M app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_lwm2m.c)
