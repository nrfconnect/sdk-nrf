#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER_EVENTS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder_event.c
)

target_sources_ifdef(CONFIG_ML_APP_ML_RESULT_EVENTS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/ml_result_event.c
)

target_sources_ifdef(CONFIG_ML_APP_MODE_EVENTS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/ml_app_mode_event.c
)

target_sources_ifdef(CONFIG_ML_APP_SENSOR_SIM_EVENTS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/sensor_sim_event.c
)
