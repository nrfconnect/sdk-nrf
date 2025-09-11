/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <health_monitor.h>

LOG_MODULE_REGISTER(nrf_ps_client_healthmon, CONFIG_NRF_PS_CLIENT_LOG_LEVEL);

#define EVENT_LOG_SIZE 5

const char *event_names[HEALTH_EVENT__COUNT] = {
	"INVALID",
	"OUT_OF_TX_BUFFERS",
	"OUT_OF_RX_BUFFERS",
	"DISCONNECTED",
	"SRP_REG",
	"NO_PREFIX",
	"WEAK_SIGNAL",
	"NO_NEIGBOURS",
	"STATS"
};

static struct counters_data last_monitor_data;
static health_event_type_t last_events[EVENT_LOG_SIZE];
static uint8_t last_event_index;

static const char *event_name(health_event_type_t event)
{
	const char *name = "UNKNOWN";

	if (event < HEALTH_EVENT__COUNT) {
		return event_names[event];
	}

	return name;
}

static void add_event(health_event_type_t event)
{
	uint8_t new_index = (last_event_index + 1) % EVENT_LOG_SIZE;

	last_events[new_index] = event;
	last_event_index = new_index;
}

static void health_monitor_event_handler(struct health_event_data *event, void *context)
{
	LOG_INF("received event %s", event_name(event->event_type));
	add_event(event->event_type);

	switch (event->event_type) {
	case HEALTH_EVENT_STATS:
		memcpy(&last_monitor_data, event->counters, sizeof(last_monitor_data));
		LOG_INF("Counter values:\n\r"
			"\tstate changes:           %u\n\r"
			"\tchild added:             %u\n\r"
			"\tchild removed:           %u\n\r"
			"\tpartition id changes:    %u\n\r"
			"\tkey sequence changes:    %u\n\r"
			"\tnetwork data changes:    %u\n\r"
			"\tactive dataset changes:  %u\n\r"
			"\tpending dataset changes: %u\n\r",
			event->counters->state_changes,
			event->counters->child_added,
			event->counters->child_removed,
			event->counters->partition_id_changes,
			event->counters->key_sequence_changes,
			event->counters->network_data_changes,
			event->counters->active_dataset_changes,
			event->counters->pending_dataset_changes);
		break;
	default:
		/* unknown or no args event. */
		break;
	}
}

static int cmd_healthmon_interval(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t interval = 0;

	interval = shell_strtoul(argv[1], 10, &rc);

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	health_monitor_set_raport_interval(interval);

	shell_print(sh, "Interval set to %u", interval);

	return 0;
}

static int cmd_healthmon_last(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "EVENTS:");
	uint8_t last_event = last_event_index;

	for (uint8_t index = last_event + EVENT_LOG_SIZE; index > last_event; index--) {
		health_event_type_t event = last_events[index % EVENT_LOG_SIZE];

		shell_print(sh, "\t%s(%d)", event_name(event), event);
	}

	shell_print(sh, "\n\rRAPORT:");
	shell_print(sh, "\tstate changes:           %u", last_monitor_data.state_changes);
	shell_print(sh, "\tchild added:             %u", last_monitor_data.child_added);
	shell_print(sh, "\tchild removed:           %u", last_monitor_data.child_removed);
	shell_print(sh, "\tpartition id changes:    %u", last_monitor_data.partition_id_changes);
	shell_print(sh, "\tkey sequence changes:    %u", last_monitor_data.key_sequence_changes);
	shell_print(sh, "\tnetwork data changes:    %u", last_monitor_data.network_data_changes);
	shell_print(sh, "\tactive dataset changes:  %u", last_monitor_data.active_dataset_changes);
	shell_print(sh, "\tpending dataset changes: %u", last_monitor_data.pending_dataset_changes);

	shell_print(sh, "done");

	return 0;
}

static int cmd_healthmon_register(const struct shell *sh, size_t argc, char *argv[])
{
	health_monitor_set_event_callback(health_monitor_event_handler, NULL);
	shell_print(sh, "Event handler registered.");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	healthmon_cmds,
	SHELL_CMD_ARG(interval, NULL, "Set the monitor interval",
		      cmd_healthmon_interval, 2, 0),
	SHELL_CMD_ARG(last, NULL, "Get last data reported by health monitor",
		      cmd_healthmon_last, 0, 0),
	SHELL_CMD_ARG(register, NULL, "Register for notification reception",
		      cmd_healthmon_register, 0, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(healthmon, &healthmon_cmds, "nRF RPC utility commands", NULL, 1, 0);
