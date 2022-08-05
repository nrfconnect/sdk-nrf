/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include "zigbee_shell_utils.h"

#define MONITOR_ON_HELP \
	("Start monitoring the list of active Zigbee neighbors.\n" \
	 "Usage: on")
#define MONITOR_OFF_HELP \
	("Stop monitoring the list of active Zigbee neighbors.\n" \
	 "Usage: off")
#define MONITOR_TRIGGER_HELP \
	("Trigger logging the list of active Zigbee neighbors.\n" \
	 "Usage: trigger")

#define IEEE_ADDR_BUF_SIZE    17
#define UPDATE_COUNT_RELOAD   0xFFFFFFFF


struct active_nbr_cache_entry {
	zb_ieee_addr_t ieee_addr;
	zb_uint8_t     device_type;
	zb_uint8_t     relationship;
	zb_uint8_t     outgoing_cost;   /*!< The cost of an outgoing link. Got from link status. */
	zb_uint8_t     age;             /*!< Counter value for router aging. */
	zb_uint32_t    timeout_counter; /*!< Timeout value ED aging, in milliseconds. */
};

struct active_nbr_cache {
	struct active_nbr_cache_entry entries[CONFIG_ZIGBEE_SHELL_MONITOR_CACHE_SIZE];
	zb_uint32_t                   n_entries;
};


/* Use constant logging level.
 * There is no point in eabling neighbor table monitor and suppressing its logs.
 */
LOG_MODULE_REGISTER(zigbee_shell_nbr, LOG_LEVEL_INF);

static struct active_nbr_cache nbr_cache;
static zb_bool_t monitor_active;
static zb_bufid_t monitor_buf;
static zb_uint32_t last_update_count = UPDATE_COUNT_RELOAD;
static const char * const device_type_name[] = {
	"coordinator",
	"router     ",
	"end device ",
	"unknown    "
};
static const char * const relationship_name[] = {
	"parent               ",
	"child                ",
	"sibling              ",
	"unknown              ",
	"former child         ",
	"unauthenticated child"
};

static void refresh_active_nbt_table(zb_bufid_t bufid);

static void active_nbr_clear(void)
{
	nbr_cache.n_entries = 0;
}

static void active_nbr_store(zb_nwk_nbr_iterator_entry_t *entry)
{
	if (nbr_cache.n_entries < CONFIG_ZIGBEE_SHELL_MONITOR_CACHE_SIZE) {
		ZB_IEEE_ADDR_COPY(
			nbr_cache.entries[nbr_cache.n_entries].ieee_addr,
			entry->ieee_addr);
		nbr_cache.entries[nbr_cache.n_entries].device_type = entry->device_type;
		nbr_cache.entries[nbr_cache.n_entries].relationship = entry->relationship;
		nbr_cache.entries[nbr_cache.n_entries].outgoing_cost = entry->outgoing_cost;
		nbr_cache.entries[nbr_cache.n_entries].age = entry->age;
		nbr_cache.entries[nbr_cache.n_entries].timeout_counter = entry->timeout_counter;

		nbr_cache.n_entries++;
	}
}

static void active_nbr_log(void)
{
	zb_uint32_t i;
	char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
	int addr_len;
	int age;
	int timeout;

	LOG_INF("Active neighbor table (%d entries):", nbr_cache.n_entries);
	LOG_INF("[idx] ext_addr         device_type relationship          cost age timeout");

	for (i = 0; i < nbr_cache.n_entries; i++) {
		addr_len =
			ieee_addr_to_str(ieee_addr_buf,
					 sizeof(ieee_addr_buf),
					 nbr_cache.entries[i].ieee_addr);
		if (addr_len < 0) {
			strcpy(ieee_addr_buf, "unknown");
		}

		ZB_ASSERT(nbr_cache.entries[i].device_type < 4);
		ZB_ASSERT(nbr_cache.entries[i].relationship < 6);

		if (nbr_cache.entries[i].device_type == 2) {
			timeout = nbr_cache.entries[i].timeout_counter;
			age = -1;
		} else {
			timeout = -1;
			age = nbr_cache.entries[i].age;
		}

		LOG_INF("[%3u] %s %s %s %4u %3d %d",
			i,
			ieee_addr_buf,
			device_type_name[nbr_cache.entries[i].device_type],
			relationship_name[nbr_cache.entries[i].relationship],
			nbr_cache.entries[i].outgoing_cost,
			age,
			timeout);
	}
}

static void refresh_active_nbt_table_delayed(zb_bufid_t bufid)
{
	zb_ret_t error_code;

	error_code = zb_nwk_nbr_iterator_next(bufid, refresh_active_nbt_table);
	ZB_ERROR_CHECK(error_code);
}

static void refresh_active_nbt_table(zb_bufid_t bufid)
{
	static zb_bool_t cache_updated = ZB_FALSE;
	zb_ret_t error_code;
	zb_nwk_nbr_iterator_params_t *args = ZB_BUF_GET_PARAM(bufid, zb_nwk_nbr_iterator_params_t);
	zb_nwk_nbr_iterator_entry_t *entry = (zb_nwk_nbr_iterator_entry_t *)zb_buf_begin(bufid);

	/* Check if the function processes the newest active request. */
	if ((bufid != monitor_buf) || (monitor_active == ZB_FALSE)) {
		zb_buf_free(bufid);
		return;
	}

	/* Initialize the params structure if this is the first API call. */
	if (last_update_count == UPDATE_COUNT_RELOAD) {
		args->update_count = 0;
		args->index = ZB_NWK_NBR_ITERATOR_INDEX_EOT;
		zb_buf_set_status(bufid, RET_OK);
	}

	error_code = zb_buf_get_status(bufid);
	ZB_ERROR_CHECK(error_code);

	if (args->update_count != last_update_count) {
		/* Counter skew - restart the read process. */
		active_nbr_clear();
		args->index = 0;
		last_update_count = args->update_count;
		cache_updated = ZB_TRUE;
	} else if (args->index != ZB_NWK_NBR_ITERATOR_INDEX_EOT) {
		/* Counter correct, regular entry - add it to the cache. */
		active_nbr_store(entry);
		args->index++;
		cache_updated = ZB_TRUE;
	} else if (cache_updated) {
		/* Counter correct, last entry received.
		 * Previous call contained a regular entry - log the table.
		 */
		active_nbr_log();
		cache_updated = ZB_FALSE;
	}

	if (args->index != ZB_NWK_NBR_ITERATOR_INDEX_EOT) {
		/* Regular entry received - call the API immediately. */
		error_code = zb_nwk_nbr_iterator_next(bufid, refresh_active_nbt_table);
		ZB_ERROR_CHECK(error_code);
	} else {
		/* Regular entry not present - call the API after 1 second. */
		ZB_SCHEDULE_APP_ALARM(
			refresh_active_nbt_table_delayed,
			bufid,
			ZB_TIME_ONE_SECOND);
	}
}

/**@brief Enable, disable or refresh neighbor table monitor.
 *
 * @code
 * nbr monitor {on,off,trigger}
 * @endcode
 *
 * Register a neighbor table monitoring routine and log all entries once
 * a new entry is created or an existing one is removed.
 * The trigger command can be used to trigger logging the current state
 * of the neighbor table.
 *
 * Example:
 * @code
 * nbr monitor on
 * @endcode
 *
 */
static int cmd_zb_nbr_monitor(const struct shell *shell, size_t argc, char **argv)
{
	zb_bool_t start;

	ARG_UNUSED(argc);
	if (strcmp(argv[0], "trigger") == 0) {
		/* This will trigger logging (if monitor is active). */
		last_update_count = UPDATE_COUNT_RELOAD;
		zb_shell_print_done(shell, ZB_FALSE);
		return 0;
	}

	if (strcmp(argv[0], "on") == 0) {
		start = ZB_TRUE;
	} else {
		start = ZB_FALSE;
	}

	if (monitor_active == start) {
		/* Make sure that the monitor is not started twice. */
		zb_shell_print_done(shell, ZB_FALSE);
		return 0;
	}

	if (start == ZB_TRUE) {
		/* Make sure ZBOSS buffer API is called safely. */
		zb_osif_disable_all_inter();
		monitor_buf = zb_buf_get_out();
		zb_osif_enable_all_inter();

		if (!monitor_buf) {
			zb_shell_print_error(
				shell,
				"Failed to execute command (buf alloc failed)",
				ZB_FALSE);
			return -ENOEXEC;
		}

		last_update_count = UPDATE_COUNT_RELOAD;
		monitor_active = ZB_TRUE;
		ZB_SCHEDULE_APP_CALLBACK(refresh_active_nbt_table, monitor_buf);
	} else if (monitor_active) {
		monitor_active = ZB_FALSE;
		/* monitor_buf will be freed from refresh_active_nbt_table. */
	}

	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_monitor,
	SHELL_CMD_ARG(on, NULL, MONITOR_ON_HELP, cmd_zb_nbr_monitor, 1, 0),
	SHELL_CMD_ARG(off, NULL, MONITOR_OFF_HELP, cmd_zb_nbr_monitor, 1, 0),
	SHELL_CMD_ARG(trigger, NULL, MONITOR_TRIGGER_HELP, cmd_zb_nbr_monitor, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_nbr,
	SHELL_CMD(monitor, &sub_monitor,
		  "Enable/disable neighbor table monitoring.", NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nbr, &sub_nbr, "Zigbee neighbor table.", NULL);
