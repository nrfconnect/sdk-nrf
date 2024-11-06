/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DESH_STARTUP_CMD_SETTINGS_H
#define DESH_STARTUP_CMD_SETTINGS_H

#define STARTUP_CMD_MAX_LEN 256
#define STARTUP_CMD_MAX_COUNT 6
#define STARTUP_CMD_STARTTIME_DEFAULT 1
#define STARTUP_CMD_DELAY_DEFAULT 5

struct startup_cmd {
	int starttime;
	int delay;
	char cmd_str[STARTUP_CMD_MAX_LEN + 1];
};
struct startup_cmd_data {
	int starttime;
	struct startup_cmd cmd[STARTUP_CMD_MAX_COUNT];
};

int startup_cmd_settings_init(void);

void startup_cmd_settings_data_print(void);
int startup_cmd_settings_data_save(struct startup_cmd_data *new_startup_cmd_data);
int startup_cmd_settings_data_read(struct startup_cmd_data *out_startup_cmd_data);

int startup_cmd_settings_cmd_data_read_by_memslot(
	struct startup_cmd *out_cmd_data, int mem_slot);

bool startup_cmd_settings_enabled(void);

int startup_cmd_settings_starttime_get(void);

int startup_cmd_settings_cmd_delay_get(int mem_slot);

#endif /* DESH_STARTUP_CMD_SETTINGS_H */
