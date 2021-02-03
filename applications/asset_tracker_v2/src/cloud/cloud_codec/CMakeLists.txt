#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources_ifdef(CONFIG_AWS_IOT app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/aws_iot_codec.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_codec_ringbuffer.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/json_aux.c)
