#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PRIVATE .)

#
# DECT-specific custom AT command wrappers (e.g. native TLS helpers).
#
target_sources_ifdef(CONFIG_AT_CMD_CUSTOM app PRIVATE at_cmd_mode_custom.c)
