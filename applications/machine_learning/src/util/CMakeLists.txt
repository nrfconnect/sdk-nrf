#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder.c)
