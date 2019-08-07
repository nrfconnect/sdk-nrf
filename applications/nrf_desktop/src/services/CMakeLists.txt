#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_DESKTOP_BAS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bas.c)

target_sources_ifdef(CONFIG_DESKTOP_HIDS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hids.c)
