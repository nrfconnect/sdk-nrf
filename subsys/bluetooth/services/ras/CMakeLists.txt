#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BT_RAS_RRSP rrsp)
add_subdirectory_ifdef(CONFIG_BT_RAS_RREQ rreq)
