/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_AT_CMD_MODE_SETT_H
#define MOSH_AT_CMD_MODE_SETT_H

int at_cmd_mode_sett_init(void);
int at_cmd_mode_sett_autostart_enabled(bool enabled);
bool at_cmd_mode_sett_is_autostart_enabled(void);

#endif /* MOSH_AT_CMD_MODE_SETT_H */
