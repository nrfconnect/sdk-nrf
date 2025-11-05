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
	"BUFFERS_LOW",
	"DISCONNECTED",
	"SRP_REG",
	"NO_PREFIX",
	"WEAK_SIGNAL",
	"NO_NEIGHBORS",
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
	if (event != HEALTH_EVENT_STATS) {
		uint8_t new_index = (last_event_index + 1) % EVENT_LOG_SIZE;

		last_events[new_index] = event;
		last_event_index = new_index;
	}
}

static void health_monitor_event_handler(struct health_event_data *event, void *context)
{
	LOG_INF("received event %s", event_name(event->event_type));
	add_event(event->event_type);

	switch (event->event_type) {
	case HEALTH_EVENT_STATS:
		memcpy(&last_monitor_data, event->counters, sizeof(last_monitor_data));
		LOG_INF("Counter values:\n\r"
			"\tchild added:                       %u\n\r"
			"\tchild removed:                     %u\n\r"
			"\tpartition id changes:              %u\n\r"
			"\tkey sequence changes:              %u\n\r"
			"\tnetwork data changes:              %u\n\r"
			"\tactive dataset changes:            %u\n\r"
			"\tpending dataset changes:           %u\n\r"
			"\trole_disabled:                     %u\n\r"
			"\trole_detached:                     %u\n\r"
			"\trole_child:                        %u\n\r"
			"\trole_router:                       %u\n\r"
			"\trole_leader:                       %u\n\r"
			"\tattach_attempts:                   %u\n\r"
			"\tbetter_partition_attach_attempts:  %u\n\r"
			"\tip_tx_success:                     %u\n\r"
			"\tip_rx_success:                     %u\n\r"
			"\tip_tx_failure:                     %u\n\r"
			"\tip_rx_failure:                     %u\n\r"
			"\tmac_tx_total:                      %u\n\r"
			"\tmac_tx_unicast:                    %u\n\r"
			"\tmac_tx_broadcast:                  %u\n\r"
			"\tmac_tx_retry:                      %u\n\r"
			"\tmac_rx_total:                      %u\n\r"
			"\tmac_rx_unicast:                    %u\n\r"
			"\tmac_rx_broadcast:                  %u\n\r"
			"\tchild_supervision_failure:         %u\n\r"
			"\tchild_max:                         %u\n\r"
			"\trouter_max:                        %u\n\r"
			"\trouter_added:                      %u\n\r"
			"\trouter_removed:                    %u\n\r"
			"\tsrp_server_changes:                %u\n\r",
			(uint32_t)event->counters->child_added,
			(uint32_t)event->counters->child_removed,
			(uint32_t)event->counters->partition_id_changes,
			(uint32_t)event->counters->key_sequence_changes,
			(uint32_t)event->counters->network_data_changes,
			(uint32_t)event->counters->active_dataset_changes,
			(uint32_t)event->counters->pending_dataset_changes,
			(uint32_t)event->counters->role_disabled,
			(uint32_t)event->counters->role_detached,
			(uint32_t)event->counters->role_child,
			(uint32_t)event->counters->role_router,
			(uint32_t)event->counters->role_leader,
			(uint32_t)event->counters->attach_attempts,
			(uint32_t)event->counters->better_partition_attach_attempts,
			(uint32_t)event->counters->ip_tx_success,
			(uint32_t)event->counters->ip_rx_success,
			(uint32_t)event->counters->ip_tx_failure,
			(uint32_t)event->counters->ip_rx_failure,
			(uint32_t)event->counters->mac_tx_unicast +
			(uint32_t)event->counters->mac_tx_broadcast,
			(uint32_t)event->counters->mac_tx_unicast,
			(uint32_t)event->counters->mac_tx_broadcast,
			(uint32_t)event->counters->mac_tx_retry,
			(uint32_t)event->counters->mac_rx_unicast +
			(uint32_t)event->counters->mac_rx_broadcast,
			(uint32_t)event->counters->mac_rx_unicast,
			(uint32_t)event->counters->mac_rx_broadcast,
			(uint32_t)event->counters->child_supervision_failure,
			(uint32_t)event->counters->child_max,
			(uint32_t)event->counters->router_max,
			(uint32_t)event->counters->router_added,
			(uint32_t)event->counters->router_removed,
			(uint32_t)event->counters->srp_server_changes
		);
		break;
	case HEALTH_EVENT_NO_BUFFERS:
		LOG_INF("Buffers:\n\r"
			"\ttotal:           %u\n\r"
			"\tmax used:             %u\n\r"
			"\tcurren free:           %u\n\r",
			event->no_buffers.total_count,
			event->no_buffers.max_used,
			event->no_buffers.current_free);
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
	shell_print(sh, "\tchild added:                       %u",
			(uint32_t)last_monitor_data.child_added);
	shell_print(sh, "\tchild removed:                     %u",
			(uint32_t)last_monitor_data.child_removed);
	shell_print(sh, "\tpartition id changes:              %u",
			(uint32_t)last_monitor_data.partition_id_changes);
	shell_print(sh, "\tkey sequence changes:              %u",
			(uint32_t)last_monitor_data.key_sequence_changes);
	shell_print(sh, "\tnetwork data changes:              %u",
			(uint32_t)last_monitor_data.network_data_changes);
	shell_print(sh, "\tactive dataset changes:            %u",
			(uint32_t)last_monitor_data.active_dataset_changes);
	shell_print(sh, "\tpending dataset changes:           %u",
			(uint32_t)last_monitor_data.pending_dataset_changes);
	shell_print(sh, "\trole_disabled:                     %u",
			(uint32_t)last_monitor_data.role_disabled);
	shell_print(sh, "\trole_detached:                     %u",
			(uint32_t)last_monitor_data.role_detached);
	shell_print(sh, "\trole_child:                        %u",
			(uint32_t)last_monitor_data.role_child);
	shell_print(sh, "\trole_router:                       %u",
			(uint32_t)last_monitor_data.role_router);
	shell_print(sh, "\trole_leader:                       %u",
			(uint32_t)last_monitor_data.role_leader);
	shell_print(sh, "\tattach_attempts:                   %u",
			(uint32_t)last_monitor_data.attach_attempts);
	shell_print(sh, "\tbetter_partition_attach_attempts:  %u",
			(uint32_t)last_monitor_data.better_partition_attach_attempts);
	shell_print(sh, "\tip_tx_success:                     %u",
			(uint32_t)last_monitor_data.ip_tx_success);
	shell_print(sh, "\tip_rx_success:                     %u",
			(uint32_t)last_monitor_data.ip_rx_success);
	shell_print(sh, "\tip_tx_failure:                     %u",
			(uint32_t)last_monitor_data.ip_tx_failure);
	shell_print(sh, "\tip_rx_failure:                     %u",
			(uint32_t)last_monitor_data.ip_rx_failure);
	shell_print(sh, "\tmac_tx_total:                      %u",
			(uint32_t)last_monitor_data.mac_tx_unicast +
			(uint32_t)last_monitor_data.mac_tx_broadcast);
	shell_print(sh, "\tmac_tx_unicast:                    %u",
			(uint32_t)last_monitor_data.mac_tx_unicast);
	shell_print(sh, "\tmac_tx_broadcast:                  %u",
			(uint32_t)last_monitor_data.mac_tx_broadcast);
	shell_print(sh, "\tmac_tx_retry:                      %u",
			(uint32_t)last_monitor_data.mac_tx_retry);
	shell_print(sh, "\tmac_rx_total:                      %u",
			(uint32_t)last_monitor_data.mac_rx_unicast +
			(uint32_t)last_monitor_data.mac_rx_broadcast);
	shell_print(sh, "\tmac_rx_unicast:                    %u",
			(uint32_t)last_monitor_data.mac_rx_unicast);
	shell_print(sh, "\tmac_rx_broadcast:                  %u",
			(uint32_t)last_monitor_data.mac_rx_broadcast);
	shell_print(sh, "\tchild_supervision_failure:         %u",
			(uint32_t)last_monitor_data.child_supervision_failure);
	shell_print(sh, "\tchild_max:                         %u",
			(uint32_t)last_monitor_data.child_max);
	shell_print(sh, "\trouter_max:                        %u",
			(uint32_t)last_monitor_data.router_max);
	shell_print(sh, "\trouter_added:                      %u",
			(uint32_t)last_monitor_data.router_added);
	shell_print(sh, "\trouter_removed:                    %u",
			(uint32_t)last_monitor_data.router_removed);
	shell_print(sh, "\tsrp_server_changes:                %u",
			(uint32_t)last_monitor_data.srp_server_changes);

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
