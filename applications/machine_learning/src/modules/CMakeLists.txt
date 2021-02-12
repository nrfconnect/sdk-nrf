#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder.c)

target_sources_ifdef(CONFIG_ML_APP_ML_RUNNER_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ml_runner.c)

target_sources_ifdef(CONFIG_ML_APP_SENSOR_SIM_CTRL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensor_sim_ctrl.c)
