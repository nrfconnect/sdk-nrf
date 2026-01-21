#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_OS os_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_IMG img_mgmt)
