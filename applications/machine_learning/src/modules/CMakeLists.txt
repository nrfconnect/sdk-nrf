#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER_UART
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder_uart.c)

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER_BT_NUS
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder_bt_nus.c)

target_sources_ifdef(CONFIG_ML_APP_LED_STATE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_ML_APP_ML_RUNNER
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ml_runner.c)

target_sources_ifdef(CONFIG_ML_APP_MODE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ml_app_mode.c)

target_sources_ifdef(CONFIG_ML_APP_USB_STATE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state.c)

target_sources_ifdef(CONFIG_ML_APP_SENSOR_SIM_CTRL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensor_sim_ctrl.c)
