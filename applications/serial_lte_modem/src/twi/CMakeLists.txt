#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources_ifdef(CONFIG_SLM_TWI app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/slm_at_twi.c)
