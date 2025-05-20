#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder.c)

target_sources_ifdef(CONFIG_ML_APP_ADV_PROV_UUID128_ALL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bt_le_adv_prov_uuid128.c)
