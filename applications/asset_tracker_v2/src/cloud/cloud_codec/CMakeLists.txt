#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources_ifdef(CONFIG_CLOUD_CODEC_AWS_IOT app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/aws_iot/aws_iot_codec.c)

target_sources_ifdef(CONFIG_CLOUD_CODEC_AZURE_IOT_HUB app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/azure_iot_hub/azure_iot_hub_codec.c)

target_sources_ifdef(CONFIG_CLOUD_CODEC_NRF_CLOUD app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nrf_cloud/nrf_cloud_codec.c)

target_sources_ifdef(CONFIG_CLOUD_CODEC_LWM2M app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m/lwm2m_codec_helpers.c
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwm2m/lwm2m_codec.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_codec_ringbuffer.c)

# Include JSON convenience APIs if used by the respective cloud codec backend.
if (CONFIG_CLOUD_CODEC_AWS_IOT OR CONFIG_CLOUD_CODEC_AZURE_IOT_HUB OR CONFIG_CLOUD_CODEC_NRF_CLOUD)
        target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/json_helpers.c)
endif()
if (CONFIG_CLOUD_CODEC_AWS_IOT OR CONFIG_CLOUD_CODEC_AZURE_IOT_HUB)
        target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/json_common.c)
endif()
