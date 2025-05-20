#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BH1749 bh1749)
add_subdirectory_ifdef(CONFIG_SENSOR_SIM sensor_sim)
add_subdirectory_ifdef(CONFIG_SENSOR_STUB sensor_stub)
add_subdirectory_ifdef(CONFIG_PMW3360 pmw3360)
add_subdirectory_ifdef(CONFIG_PAW3212 paw3212)
add_subdirectory_ifdef(CONFIG_BME68X_IAQ bme68x_iaq)
