#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_LWM2M_CARRIER lwm2m_carrier)
add_subdirectory_ifdef(CONFIG_SB_FOTA sb_fota)
