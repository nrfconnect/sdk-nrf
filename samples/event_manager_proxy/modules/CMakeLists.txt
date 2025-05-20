#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_sources_ifdef(CONFIG_APP_MODULE_CONTROLLER_ENABLE       app PRIVATE controller.c)
target_sources_ifdef(CONFIG_APP_MODULE_SENSOR_SIMULATED_ENABLE app PRIVATE sensor_simulated.c)
target_sources_ifdef(CONFIG_APP_MODULE_STATS_ENABLE            app PRIVATE stats.c)
