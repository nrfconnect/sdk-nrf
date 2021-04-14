#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder.c)

target_sources_ifdef(CONFIG_ML_APP_LED_STATE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_ML_APP_ML_RUNNER
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ml_runner.c)

target_sources_ifdef(CONFIG_ML_APP_ML_STATE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ml_state.c)

target_sources_ifdef(CONFIG_ML_APP_SENSOR_SIM_CTRL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensor_sim_ctrl.c)
