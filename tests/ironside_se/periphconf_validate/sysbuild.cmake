# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

ExternalZephyrProject_Add(
	APPLICATION validate_rad
	SOURCE_DIR  ${APP_DIR}/validate_rad
	BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
)
