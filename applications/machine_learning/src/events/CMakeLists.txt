#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_ML_APP_ML_RESULT_EVENTS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/ml_result_event.c
)
