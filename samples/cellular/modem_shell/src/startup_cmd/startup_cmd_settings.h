/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_STARTUP_CMD_SETTINGS_H
#define MOSH_STARTUP_CMD_SETTINGS_H

#define STARTUP_CMD_MAX_LEN 128
#define STARTUP_CMD_MAX_COUNT 3
#define STARTUP_CMD_STARTTIME_NOT_SET -1

int startup_cmd_settings_init(void);
void startup_cmd_settings_conf_shell_print(void);
int startup_cmd_settings_save(const char *cmd_str, int mem_slot);
char *startup_cmd_settings_get(uint8_t mem_slot);
bool startup_cmd_settings_enabled(void);

int startup_cmd_settings_starttime_set(int starttime);
int startup_cmd_settings_starttime_get(void);

#endif /* MOSH_STARTUP_CMD_SETTINGS_H */
