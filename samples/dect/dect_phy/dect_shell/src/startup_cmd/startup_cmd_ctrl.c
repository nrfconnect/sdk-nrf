/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/shell/shell.h>

#include "desh_print.h"
#include "startup_cmd_settings.h"

extern struct k_work_q desh_common_work_q;
extern const struct shell *desh_shell;

/* Work for running commands: */
struct startup_cmd_worker_data {
	struct k_work_delayable work;
	uint8_t running_mem_slot;
};
static struct startup_cmd_worker_data startup_cmd_worker_data;

static bool running;

static void startup_cmd_worker(struct k_work *work_item)
{
	int len = 0;
	int err;
	struct k_work_delayable *delayable_work = k_work_delayable_from_work(work_item);
	struct startup_cmd_worker_data *data_ptr =
		CONTAINER_OF(delayable_work, struct startup_cmd_worker_data, work);
	struct startup_cmd current_cmd_data;

	err = startup_cmd_settings_cmd_data_read_by_memslot(
		&current_cmd_data, data_ptr->running_mem_slot);
	if (err) {
		desh_error("(%s): Cannot read startup_cmd data for mem_slot %d",
			(__func__), data_ptr->running_mem_slot);
		return;
	}

	len = strlen(current_cmd_data.cmd_str);
	if (len) {
		desh_print("Starting to execute sett_cmd %d \"%s\"...",
			data_ptr->running_mem_slot, current_cmd_data.cmd_str);
		shell_execute_cmd(desh_shell, current_cmd_data.cmd_str);
	}

	/* We are done for this one. Schedule next stored command. */
	if (data_ptr->running_mem_slot < STARTUP_CMD_MAX_COUNT) {
		startup_cmd_worker_data.running_mem_slot++;
		k_work_schedule_for_queue(
			&desh_common_work_q,
			&startup_cmd_worker_data.work,
			K_SECONDS(startup_cmd_settings_cmd_delay_get(
				startup_cmd_worker_data.running_mem_slot)));
	} else {
		/* No more commands to run */
		startup_cmd_worker_data.running_mem_slot = 0;
	}
}

static bool startup_cmd_can_be_started(void)
{
	return (!running && startup_cmd_settings_enabled());
}

void startup_cmd_ctrl_settings_loaded(void)
{
	int starttime = startup_cmd_settings_starttime_get();

	if (startup_cmd_can_be_started()) {
		startup_cmd_worker_data.running_mem_slot = 1;
		k_work_schedule_for_queue(
			&desh_common_work_q, &startup_cmd_worker_data.work, K_SECONDS(starttime));
		running = true;
	}
}

void startup_cmd_ctrl_init(void)
{
	k_work_init_delayable(&startup_cmd_worker_data.work, startup_cmd_worker);
	startup_cmd_settings_init();
}
