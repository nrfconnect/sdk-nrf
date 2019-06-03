#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_NRF_COAP_LIB coap)
add_subdirectory_ifdef(CONFIG_MQTT_SOCKET_LIB mqtt_socket)
add_subdirectory_ifdef(CONFIG_NRF_CLOUD nrf_cloud)
add_subdirectory_ifdef(CONFIG_NRF_DOWNLOAD_CLIENT download_client)
add_subdirectory_ifdef(CONFIG_FOTA_DOWNLOAD fota_download)
