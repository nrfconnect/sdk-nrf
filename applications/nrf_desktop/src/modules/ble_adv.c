/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#define MODULE ble_adv
#include "module_state_event.h"
#include "ble_event.h"
#include "power_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_ADV_LOG_LEVEL);

#ifndef CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT
#define CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT 0
#endif

#ifndef CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD
#define CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD 0
#endif


#define DEVICE_NAME	CONFIG_DESKTOP_BLE_SHORT_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

#define SWIFT_PAIR_SECTION_SIZE 1 /* number of struct bt_data objects */

#define MAX_KEY_LEN 30
#define PEER_IS_RPA_STORAGE_NAME "peer_is_rpa_"


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
#if CONFIG_DESKTOP_HIDS_ENABLE
			  0x12, 0x18,	/* HID Service */
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
			  0x0f, 0x18,	/* Battery Service */
#endif
	),

	BT_DATA(BT_DATA_NAME_SHORTENED, DEVICE_NAME, DEVICE_NAME_LEN),
#if CONFIG_DESKTOP_BLE_SWIFT_PAIR
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
			  0x06, 0x00,	/* Microsoft Vendor ID */
			  0x03,		/* Microsoft Beacon ID */
			  0x00,		/* Microsoft Beacon Sub Scenario */
			  0x80),	/* Reserved RSSI Byte */
#endif
};


enum state {
	STATE_DISABLED,
	STATE_OFF,
	STATE_IDLE,
	STATE_ACTIVE_FAST,
	STATE_ACTIVE_SLOW,
	STATE_ACTIVE_FAST_DIRECT,
	STATE_ACTIVE_SLOW_DIRECT,
	STATE_DELAYED_ACTIVE_FAST,
	STATE_DELAYED_ACTIVE_SLOW,
	STATE_GRACE_PERIOD
};

struct bond_find_data {
	bt_addr_le_t peer_address;
	u8_t peer_id;
	u8_t peer_count;
};

/* When using BT_LE_ADV_OPT_USE_NAME, device name is added to scan response
 * data by controller.
 */
static const struct bt_data sd[] = {};

static enum state state;
static bool adv_swift_pair;

static struct k_delayed_work adv_update;
static struct k_delayed_work sp_grace_period_to;
static u8_t cur_identity = BT_ID_DEFAULT; /* We expect zero */

enum peer_rpa {
	PEER_RPA_ERASED,
	PEER_RPA_YES,
	PEER_RPA_NO,
};

static enum peer_rpa peer_is_rpa[CONFIG_BT_ID_MAX];


static void broadcast_adv_state(bool active)
{
	struct ble_peer_search_event *event = new_ble_peer_search_event();
	event->active = active;
	EVENT_SUBMIT(event);

	LOG_INF("Advertising %s", (active)?("started"):("stopped"));
}

static int ble_adv_stop(void)
{
	int err = bt_le_adv_stop();
	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
	} else {
		k_delayed_work_cancel(&adv_update);

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR) &&
		    IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_ENABLE)) {
			k_delayed_work_cancel(&sp_grace_period_to);
		}

		state = STATE_IDLE;

		broadcast_adv_state(false);
	}

	return err;
}

static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	struct bond_find_data *bond_find_data = user_data;

	if (bond_find_data->peer_id == bond_find_data->peer_count) {
		bt_addr_le_copy(&bond_find_data->peer_address, &info->addr);
	}

	__ASSERT_NO_MSG(bond_find_data->peer_count < UCHAR_MAX);
	bond_find_data->peer_count++;
}

static int ble_adv_start_directed(const bt_addr_le_t *addr, bool fast_adv)
{
	struct bt_le_adv_param adv_param;

	if (fast_adv) {
		LOG_INF("Use fast advertising");
		adv_param = *BT_LE_ADV_CONN_DIR;
	} else {
		adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY;
	}

	adv_param.id = cur_identity;

	struct bt_conn *conn = bt_conn_create_slave_le(addr, &adv_param);

	if (conn == NULL) {
		return -ENOMEM;
	}

	bt_conn_unref(conn);

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	LOG_INF("Direct advertising to %s", log_strdup(addr_str));

	return 0;
}

static int ble_adv_start_undirected(const bt_addr_le_t *bond_addr,
				    bool fast_adv, bool swift_pair)
{
	struct bt_le_adv_param adv_param = {
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME |
			   BT_LE_ADV_OPT_USE_NAME,
	};

	LOG_INF("Use %s advertising", (fast_adv)?("fast"):("slow"));
	if (fast_adv) {
		adv_param.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
		adv_param.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
	} else {
		adv_param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
		adv_param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	}

	if (IS_ENABLED(CONFIG_BT_WHITELIST)) {
		int err = bt_le_whitelist_clear();

		if (err) {
			LOG_ERR("Cannot clear whitelist (err: %d)", err);
			return err;
		}

		if (bt_addr_le_cmp(bond_addr, BT_ADDR_LE_ANY)) {
			adv_param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
			adv_param.options |= BT_LE_ADV_OPT_FILTER_CONN;
			err = bt_le_whitelist_add(bond_addr);
		}

		if (err) {
			LOG_ERR("Cannot add peer to whitelist (err: %d)", err);
			return err;
		}
	}

	adv_param.id = cur_identity;
	size_t ad_size = ARRAY_SIZE(ad);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		adv_swift_pair = swift_pair;
		if (!swift_pair) {
			ad_size = ARRAY_SIZE(ad) - SWIFT_PAIR_SECTION_SIZE;
		}
	}

	return bt_le_adv_start(&adv_param, ad, ad_size, sd, ARRAY_SIZE(sd));
}

static int ble_adv_start(bool can_fast_adv)
{
	bool fast_adv = IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV) && can_fast_adv;

	struct bond_find_data bond_find_data = {
		.peer_id = 0,
		.peer_count = 0,
	};
	bt_addr_le_copy(&bond_find_data.peer_address, BT_ADDR_LE_ANY);
	bt_foreach_bond(cur_identity, bond_find, &bond_find_data);

	int err = ble_adv_stop();
	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
		goto error;
	}

	bool direct = false;
	bool swift_pair = true;

	if (bond_find_data.peer_id < bond_find_data.peer_count) {
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_DIRECT_ADV)) {
			/* Direct advertising only to peer without RPA. */
			direct = (peer_is_rpa[cur_identity] != PEER_RPA_YES);
		}

		swift_pair = false;
	}

	if (direct) {
		err = ble_adv_start_directed(&bond_find_data.peer_address,
					     fast_adv);
	} else {
		err = ble_adv_start_undirected(&bond_find_data.peer_address,
					       fast_adv, swift_pair);
	}

	if (err == -ECONNREFUSED || (err == -ENOMEM)) {
		LOG_WRN("Already connected, do not advertise");
		err = 0;
		goto error;
	} else if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		goto error;
	}

	if (direct) {
		if (fast_adv) {
			state = STATE_ACTIVE_FAST_DIRECT;
		} else {
			state = STATE_ACTIVE_SLOW_DIRECT;
		}
	} else {
		if (fast_adv) {
			k_delayed_work_submit(&adv_update,
					      K_SECONDS(CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT));
			state = STATE_ACTIVE_FAST;
		} else {
			state = STATE_ACTIVE_SLOW;
		}
	}

	broadcast_adv_state(true);
error:
	return err;
}

static void sp_grace_period_fn(struct k_work *work)
{
	int err = ble_adv_stop();

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		state = STATE_OFF;
		module_set_state(MODULE_STATE_OFF);
	}
}

static int remove_swift_pair_section(void)
{
	int err = bt_le_adv_update_data(ad, (ARRAY_SIZE(ad) - SWIFT_PAIR_SECTION_SIZE),
					sd, ARRAY_SIZE(sd));

	if (!err) {
		LOG_INF("Swift Pair section removed");
		adv_swift_pair = false;

		k_delayed_work_cancel(&adv_update);

		k_delayed_work_submit(&sp_grace_period_to,
				      K_SECONDS(CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD));

		state = STATE_GRACE_PERIOD;
	} else if (err == -EAGAIN) {
		LOG_INF("No active advertising");
		err = ble_adv_stop();
		if (!err) {
			state = STATE_OFF;
			module_set_state(MODULE_STATE_OFF);
		}
	} else {
		LOG_ERR("Cannot modify advertising data (err %d)", err);
	}

	return err;
}

static void ble_adv_update_fn(struct k_work *work)
{
	bool can_fast_adv = false;

	switch (state) {
	case STATE_DELAYED_ACTIVE_FAST:
		can_fast_adv = true;
		break;

	case STATE_ACTIVE_FAST:
	case STATE_DELAYED_ACTIVE_SLOW:
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}

	int err = ble_adv_start(can_fast_adv);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	/* Assuming ID is written as one digit */
	if (!strncmp(key, PEER_IS_RPA_STORAGE_NAME,
	     sizeof(PEER_IS_RPA_STORAGE_NAME) - 1)) {
		char *end;
		long int read_id = strtol(key + strlen(key) - 1, &end, 10);

		if ((*end != '\0') || (read_id < 0) || (read_id >= CONFIG_BT_ID_MAX)) {
			LOG_ERR("Identity is not a valid number");
			return -ENOTSUP;
		}

		ssize_t len = read_cb(cb_arg, &peer_is_rpa[read_id],
				  sizeof(peer_is_rpa[read_id]));

		if (len != sizeof(peer_is_rpa[read_id])) {
			LOG_ERR("Can't read peer_is_rpa%ld from storage", read_id);
			return len;
		}
	}

	return 0;
}

static int init_settings(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS) &&
	    IS_ENABLED(CONFIG_DESKTOP_BLE_DIRECT_ADV)) {
		static struct settings_handler sh = {
			.name = MODULE_NAME,
			.h_set = settings_set,
		};

		int err = settings_register(&sh);
		if (err) {
			LOG_ERR("Cannot register settings handler (err %d)",
				err);
			return err;
		}
	}

	return 0;
}

static void init(void)
{
	if (init_settings()) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	k_delayed_work_init(&adv_update, ble_adv_update_fn);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR) &&
	    IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_ENABLE)) {
		k_delayed_work_init(&sp_grace_period_to, sp_grace_period_fn);
	}

	/* We should not start advertising before ble_bond is ready */
	state = STATE_OFF;
	module_set_state(MODULE_STATE_READY);
}

static void start(void)
{
	int err = ble_adv_start(true);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}

	return;
}

static void update_peer_is_rpa(enum peer_rpa new_peer_rpa)
{
	if (IS_ENABLED(CONFIG_SETTINGS) &&
	    IS_ENABLED(CONFIG_DESKTOP_BLE_DIRECT_ADV)) {
		peer_is_rpa[cur_identity] = new_peer_rpa;
		/* Assuming ID is written using only one digit. */
		__ASSERT_NO_MSG(cur_identity < 10);
		char key[MAX_KEY_LEN];
		int err = snprintk(key, sizeof(key), MODULE_NAME "/%s%d",
				   PEER_IS_RPA_STORAGE_NAME, cur_identity);

		if ((err > 0) && (err < MAX_KEY_LEN)) {
			err = settings_save_one(key, &peer_is_rpa[cur_identity],
					sizeof(peer_is_rpa[cur_identity]));
		}

		if (err) {
			LOG_ERR("Problem storing peer_is_rpa: (err = %d)", err);
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);

			init();
			initialized = true;
		} else if (check_state(event, MODULE_ID(ble_bond), MODULE_STATE_READY)) {
			static bool started;

			__ASSERT_NO_MSG(!started);

			/* Settings need to be loaded before advertising start */
			start();
			started = true;
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(eh);
		int err = 0;
		bool can_fast_adv = false;

		switch (event->state) {
		case PEER_STATE_CONNECTED:
			err = ble_adv_stop();
			break;

		case PEER_STATE_SECURED:
			if (peer_is_rpa[cur_identity] == PEER_RPA_ERASED) {
				if (bt_addr_le_is_rpa(bt_conn_get_dst(event->id))) {
					update_peer_is_rpa(PEER_RPA_YES);
				} else {
					update_peer_is_rpa(PEER_RPA_NO);
				}
			}
			break;

		case PEER_STATE_DISCONNECTED:
			can_fast_adv = true;
			/* Fall-through */

		case PEER_STATE_CONN_FAILED:
			if (state != STATE_OFF) {
				state = can_fast_adv ?
					STATE_DELAYED_ACTIVE_FAST :
					STATE_DELAYED_ACTIVE_SLOW;

				k_delayed_work_submit(&adv_update, 0);
			}
			break;

		default:
			/* Ignore */
			break;
		}

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		}

		return false;
	}

	if (is_ble_peer_operation_event(eh)) {
		struct ble_peer_operation_event *event =
			cast_ble_peer_operation_event(eh);
		int err;

		switch (event->op)  {
		case PEER_OPERATION_SELECTED:
		case PEER_OPERATION_ERASE_ADV:
		case PEER_OPERATION_ERASE_ADV_CANCEL:
			if ((state == STATE_OFF) || (state == STATE_GRACE_PERIOD)) {
				cur_identity = event->bt_stack_id;
				__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);
				break;
			}

			err = ble_adv_stop();

			/* Disconnect an old identity. */
			struct bond_find_data bond_find_data = {
				.peer_id = 0,
				.peer_count = 0,
			};
			bt_foreach_bond(cur_identity, bond_find,
					&bond_find_data);
			__ASSERT_NO_MSG(bond_find_data.peer_count <= 1);

			struct bt_conn *conn = NULL;

			if (bond_find_data.peer_count > 0) {
				conn = bt_conn_lookup_addr_le(cur_identity,
						&bond_find_data.peer_address);
				if (conn) {
					bt_conn_disconnect(conn,
					    BT_HCI_ERR_REMOTE_USER_TERM_CONN);
					bt_conn_unref(conn);
				}
			}

			cur_identity = event->bt_stack_id;
			__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);

			if (event->op == PEER_OPERATION_ERASE_ADV) {
				update_peer_is_rpa(PEER_RPA_ERASED);
			}
			if (!conn) {
				err = ble_adv_start(true);
			}
			break;

		case PEER_OPERATION_ERASED:
			__ASSERT_NO_MSG(cur_identity == event->bt_stack_id);
			__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);
			break;

		case PEER_OPERATION_SELECT:
		case PEER_OPERATION_ERASE:
		case PEER_OPERATION_CANCEL:
			/* Ignore */
			break;

		case PEER_OPERATION_SCAN_REQUEST:
		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_ENABLE)) {
		if (is_power_down_event(eh)) {
			int err = 0;

			switch (state) {
			case STATE_ACTIVE_FAST:
			case STATE_ACTIVE_SLOW:
				if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR) &&
				    adv_swift_pair) {
					err = remove_swift_pair_section();
				} else {
					err = ble_adv_stop();
					if (!err) {
						state = STATE_OFF;
						module_set_state(MODULE_STATE_OFF);
					}
				}
				break;

			case STATE_DELAYED_ACTIVE_FAST:
			case STATE_DELAYED_ACTIVE_SLOW:
			case STATE_ACTIVE_FAST_DIRECT:
			case STATE_ACTIVE_SLOW_DIRECT:
				err = ble_adv_stop();
				if (!err) {
					state = STATE_OFF;
					module_set_state(MODULE_STATE_OFF);
				}
				break;

			case STATE_IDLE:
				state = STATE_OFF;
				module_set_state(MODULE_STATE_OFF);
				break;

			case STATE_OFF:
			case STATE_GRACE_PERIOD:
			case STATE_DISABLED:
				/* No action */
				break;

			default:
				/* Should never happen */
				__ASSERT_NO_MSG(false);
				break;
			}

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			}

			return state != STATE_OFF;
		}

		if (is_wake_up_event(eh)) {
			bool was_off = false;
			int err;

			switch (state) {
			case STATE_OFF:
				was_off = true;
				state = STATE_IDLE;
				/* Fall-through */

			case STATE_GRACE_PERIOD:
				err = ble_adv_start(true);

				if (err) {
					module_set_state(MODULE_STATE_ERROR);
				} else if (was_off) {
					module_set_state(MODULE_STATE_READY);
				}
				break;

			case STATE_IDLE:
			case STATE_ACTIVE_FAST:
			case STATE_ACTIVE_SLOW:
			case STATE_ACTIVE_FAST_DIRECT:
			case STATE_ACTIVE_SLOW_DIRECT:
			case STATE_DELAYED_ACTIVE_FAST:
			case STATE_DELAYED_ACTIVE_SLOW:
			case STATE_DISABLED:
				/* No action */
				break;

			default:
				__ASSERT_NO_MSG(false);
				break;
			}

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
