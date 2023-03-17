#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The nRF Cloud library folder contains multiple services
if (DEFINED CONFIG_NRF_CLOUD_MQTT OR
    DEFINED CONFIG_NRF_CLOUD_AGPS OR
    DEFINED CONFIG_NRF_CLOUD_PGPS OR
    DEFINED CONFIG_NRF_CLOUD_REST OR
    DEFINED CONFIG_NRF_CLOUD_FOTA OR
    DEFINED CONFIG_NRF_CLOUD_COAP OR
    DEFINED CONFIG_NRF_CLOUD_LOCATION)
        add_subdirectory(nrf_cloud)
endif()

add_subdirectory_ifdef(CONFIG_REST_CLIENT rest_client)
add_subdirectory_ifdef(CONFIG_DOWNLOAD_CLIENT download_client)
add_subdirectory_ifdef(CONFIG_FOTA_DOWNLOAD fota_download)
add_subdirectory_ifdef(CONFIG_AWS_JOBS aws_jobs)
add_subdirectory_ifdef(CONFIG_AWS_FOTA aws_fota)
add_subdirectory_ifdef(CONFIG_AWS_IOT aws_iot)
add_subdirectory_ifdef(CONFIG_AZURE_FOTA azure_fota)
add_subdirectory_ifdef(CONFIG_AZURE_IOT_HUB azure_iot_hub)
add_subdirectory_ifdef(CONFIG_ZZHC zzhc)
add_subdirectory_ifdef(CONFIG_ICAL_PARSER icalendar_parser)
add_subdirectory_ifdef(CONFIG_FTP_CLIENT ftp_client)
add_subdirectory_ifdef(CONFIG_COAP_UTILS coap_utils)
add_subdirectory_ifdef(CONFIG_LWM2M_CLIENT_UTILS lwm2m_client_utils)
add_subdirectory_ifdef(CONFIG_WIFI_CREDENTIALS wifi_credentials)
add_subdirectory_ifdef(CONFIG_WIFI_MGMT_EXT wifi_mgmt_ext)
add_subdirectory_ifdef(CONFIG_MQTT_HELPER mqtt_helper)
add_subdirectory_ifdef(CONFIG_NRF_PROVISIONING nrf_provisioning)
