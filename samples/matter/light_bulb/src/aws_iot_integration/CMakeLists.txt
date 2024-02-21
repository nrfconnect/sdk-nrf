#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CONFIG_AWS_IOT_INTEGRATION)
	target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/codec)

	target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/aws_iot_integration.c)
	target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/codec/codec.c)
endif()
