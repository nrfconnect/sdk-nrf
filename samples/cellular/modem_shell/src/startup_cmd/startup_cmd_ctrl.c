/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/shell/shell.h>

#include "mosh_print.h"
#include "startup_cmd_settings.h"

extern struct k_work_q mosh_common_work_q;
extern const struct shell *mosh_shell;

/* Work for running commands: */
struct startup_cmd_worker_data {
	struct k_work_delayable work;
	uint8_t running_mem_slot;
};
static struct startup_cmd_worker_data startup_cmd_worker_data;

static bool running;

static void startup_cmd_worker(struct k_work *work_item)
{
	char *shell_cmd_str;
	int len = 0;
	struct startup_cmd_worker_data *data_ptr =
		CONTAINER_OF(work_item, struct startup_cmd_worker_data, work);

	shell_cmd_str = startup_cmd_settings_get(data_ptr->running_mem_slot);

	len = strlen(shell_cmd_str);
	if (len) {
		mosh_print("Starting to execute sett_cmd %d \"%s\"...",
			data_ptr->running_mem_slot, shell_cmd_str);
		shell_execute_cmd(mosh_shell, shell_cmd_str);
	}

	/* We are done for this one. Schedule next stored command. */
	if (data_ptr->running_mem_slot < STARTUP_CMD_MAX_COUNT) {
		startup_cmd_worker_data.running_mem_slot++;
		k_work_schedule_for_queue(
			&mosh_common_work_q, &startup_cmd_worker_data.work, K_NO_WAIT);
	} else {
		/* No more commands to run */
		startup_cmd_worker_data.running_mem_slot = 0;
	}
}

static bool startup_cmd_can_be_started(void)
{
	return (!running && startup_cmd_settings_enabled());
}

void startup_cmd_ctrl_default_pdn_active(void)
{
	if (startup_cmd_can_be_started()) {
		startup_cmd_worker_data.running_mem_slot = 1;

		/* Give some time for the link module for getting status etc. */
		k_work_schedule_for_queue(&mosh_common_work_q, &startup_cmd_worker_data.work,
					  K_SECONDS(2));
		running = true;
	}
}

void startup_cmd_ctrl_settings_loaded(void)
{
	int starttime = startup_cmd_settings_starttime_get();

	if (startup_cmd_can_be_started() && starttime >= 0) {
		startup_cmd_worker_data.running_mem_slot = 1;
		k_work_schedule_for_queue(&mosh_common_work_q, &startup_cmd_worker_data.work,
					  K_SECONDS(starttime));
		running = true;
	}
}

void startup_cmd_ctrl_init(void)
{
	k_work_init_delayable(&startup_cmd_worker_data.work, startup_cmd_worker);
	startup_cmd_settings_init();
}
