/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

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


static const struct bt_le_adv_param adv_param_fast = {
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME |
		   BT_LE_ADV_OPT_USE_NAME,
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
};

static const struct bt_le_adv_param adv_param_normal = {
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME |
		   BT_LE_ADV_OPT_USE_NAME,
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
};

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

static struct k_delayed_work adv_update;
static struct k_delayed_work sp_grace_period_to;


static int ble_adv_stop(void)
{
	int err = bt_le_adv_stop();
	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
	} else {
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV)) {
			k_delayed_work_cancel(&adv_update);
		}
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR) &&
		    IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_ENABLE)) {
			k_delayed_work_cancel(&sp_grace_period_to);
		}

		state = STATE_IDLE;

		LOG_INF("Advertising stopped");
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
	const struct bt_le_adv_param *adv_param = BT_LE_ADV_CONN_DIR_LOW_DUTY;

	if (fast_adv) {
		LOG_INF("Use fast advertising");
		adv_param = BT_LE_ADV_CONN_DIR;
	}

	struct bt_conn *conn = bt_conn_create_slave_le(addr, adv_param);

	if (conn == NULL) {
		return -EFAULT;
	}

	bt_conn_unref(conn);

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	LOG_INF("Direct advertising to %s", log_strdup(addr_str));

	return 0;
}

static int ble_adv_start_undirected(bool fast_adv)
{
	const struct bt_le_adv_param *adv_param = &adv_param_normal;

	LOG_INF("Use %s advertising", (fast_adv)?("fast"):("slow"));
	if (fast_adv) {
		adv_param = &adv_param_fast;
	}

	return bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

static int ble_adv_start(bool can_fast_adv)
{
	bool fast_adv = IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV) && can_fast_adv;

	struct bond_find_data bond_find_data = {
		.peer_id = 0,
	};
	bt_foreach_bond(BT_ID_DEFAULT, bond_find, &bond_find_data);

	int err = ble_adv_stop();
	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
		goto error;
	}

	bool direct = false;
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DIRECT_ADV) &&
	    (bond_find_data.peer_id < bond_find_data.peer_count)) {
		err = ble_adv_start_directed(&bond_find_data.peer_address,
					     fast_adv);
		direct = true;
	} else {
		err = ble_adv_start_undirected(fast_adv);
	}

	if (err == -ECONNREFUSED) {
		LOG_WRN("Already connected, do not advertise");
		err = 0;
		goto error;
	} else if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		goto error;
	}

	if (direct) {
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV)) {
			state = STATE_ACTIVE_FAST_DIRECT;
		} else {
			state = STATE_ACTIVE_SLOW_DIRECT;
		}
	} else {
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV)) {
			k_delayed_work_submit(&adv_update,
					      K_SECONDS(CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT));
			state = STATE_ACTIVE_FAST;
		} else {
			state = STATE_ACTIVE_SLOW;
		}
	}

	LOG_INF("Advertising started");

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

static int remove_vendor_section(void)
{
	int err = bt_le_adv_update_data(ad, (ARRAY_SIZE(ad) - SWIFT_PAIR_SECTION_SIZE),
					sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("Cannot modify advertising data (err %d)", err);
	} else {
		LOG_INF("Vendor section removed");

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV)) {
			k_delayed_work_cancel(&adv_update);
		}

		k_delayed_work_submit(&sp_grace_period_to,
				      K_SECONDS(CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD));

		state = STATE_GRACE_PERIOD;
	}

	return err;
}

static void ble_adv_update_fn(struct k_work *work)
{
	__ASSERT_NO_MSG(state == STATE_ACTIVE_FAST);

	int err = ble_adv_start(false);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}
}

static void init(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_FAST_ADV)) {
		k_delayed_work_init(&adv_update, ble_adv_update_fn);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR) &&
	    IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_ENABLE)) {
		k_delayed_work_init(&sp_grace_period_to, sp_grace_period_fn);
	}

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

		case PEER_STATE_DISCONNECTED:
			can_fast_adv = true;
			/* fall through */

		case PEER_STATE_CONN_FAILED:
			if (state != STATE_OFF) {
				err = ble_adv_start(can_fast_adv);
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

	if (IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_ENABLE)) {
		if (is_power_down_event(eh)) {
			int err = 0;

			switch (state) {
			case STATE_ACTIVE_FAST:
			case STATE_ACTIVE_SLOW:
				if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
					err = remove_vendor_section();
				} else {
					err = ble_adv_stop();
					if (!err) {
						state = STATE_OFF;
						module_set_state(MODULE_STATE_OFF);
					}
				}
				break;

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
				/* No action */
				break;

			case STATE_DISABLED:
				/* Should never happen */
				__ASSERT_NO_MSG(false);
				break;

			default:
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
				/* fall through */

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
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
