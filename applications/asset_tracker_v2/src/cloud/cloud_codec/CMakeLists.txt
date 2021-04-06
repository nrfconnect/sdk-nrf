#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources_ifdef(CONFIG_AWS_IOT app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/aws_iot_codec.c)

target_sources_ifdef(CONFIG_AZURE_IOT_HUB app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/azure_iot_hub_codec.c)

target_sources_ifdef(CONFIG_NRF_CLOUD app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nrf_cloud_codec.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_codec_ringbuffer.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/json_helpers.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/json_common.c)
