#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_ML_APP_EI_DATA_FORWARDER_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ei_data_forwarder.c)
