#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BH1749 bh1749)
add_subdirectory_ifdef(CONFIG_SENSOR_SIM sensor_sim)
add_subdirectory_ifdef(CONFIG_PMW3360 pmw3360)
add_subdirectory_ifdef(CONFIG_PAW3212 paw3212)
