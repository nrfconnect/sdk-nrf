#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config_channel.c)
