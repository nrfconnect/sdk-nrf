#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

target_sources_ifdef(CONFIG_APP_SMP_CLIENT_FOTA_EXTERNAL
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/client.c)
