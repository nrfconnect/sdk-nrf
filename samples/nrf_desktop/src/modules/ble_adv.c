/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>

#define MODULE ble_adv
#include "module_state_event.h"
#include "ble_event.h"
#include "button_event.h"
#include "power_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_ADV_LOG_LEVEL);

#define DEVICE_NAME	CONFIG_DESKTOP_BLE_SHORT_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

#define SWIFT_PAIR_SECTION_SIZE			1 /* number of struct bt_data objects */
#define SWIFT_PAIR_SECTION_REMOVE_TIMEOUT	(CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT	\
						- CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT	\
						- CONFIG_DESKTOP_BLE_SWIFT_PAIR_REMOVE_BEFORE_EXIT_TIMEOUT)
#if CONFIG_DESKTOP_BLE_SWIFT_PAIR
#if SWIFT_PAIR_SECTION_REMOVE_TIMEOUT < 0
#error "Incorrect Swift Pair timeout configuration"
#endif
#endif /* CONFIG_DESKTOP_BLE_SWIFT_PAIR */

static const struct bt_le_adv_param adv_param_fast = {
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
};

static const struct bt_le_adv_param adv_param_normal = {
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
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

/* When using BT_LE_ADV_OPT_USE_NAME, device name is added to scan response
 * data by controller.
 */
static const struct bt_data sd[] = {};

static bool bonds_initialized;
static bool bonds_remove;

static struct k_delayed_work adv_update;
static struct k_delayed_work vendor_section_remove;

static void vendor_section_remove_fn(struct k_work *work)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		LOG_INF("Removing vendor section");
		int err = bt_le_adv_update_data(
			ad, (ARRAY_SIZE(ad) - SWIFT_PAIR_SECTION_SIZE),
			sd, ARRAY_SIZE(sd));

		if (err) {
			LOG_WRN("Cannot modify advertising data (err %d)", err);
		}
	}
}

static void ble_adv_update_fn(struct k_work *work)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		LOG_INF("Switch to normal cadence advertising");
		int err = bt_le_adv_stop();

		if (!err) {
			err = bt_le_adv_start(&adv_param_normal,
				ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		}

		if (err == -ECONNREFUSED) {
			LOG_WRN("Already connected, do not advertise");
		} else if (err) {
			LOG_ERR("Failed to restart advertising (err %d)", err);
			k_delayed_work_cancel(&vendor_section_remove);

			module_set_state(MODULE_STATE_ERROR);
			return;
		}

		k_delayed_work_submit(&vendor_section_remove,
			K_SECONDS(SWIFT_PAIR_SECTION_REMOVE_TIMEOUT));
	}
}

static int ble_adv_start(void)
{
	const struct bt_le_adv_param *adv_param = &adv_param_normal;
	int err;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		err = bt_le_adv_stop();
		if (err) {
			LOG_ERR("Cannot stop advertising (err %d)", err);
			return err;
		}

		LOG_INF("Swift pair enabled, use fast advertising");
		adv_param = &adv_param_fast;
	}

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}

	LOG_INF("Advertising started");

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		k_delayed_work_submit(&adv_update,
			K_SECONDS(CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT));
	}

	return 0;
}

static void init(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		k_delayed_work_init(&adv_update, ble_adv_update_fn);
		k_delayed_work_init(&vendor_section_remove,
				    vendor_section_remove_fn);
	}

	module_set_state(MODULE_STATE_READY);
}

static void start(void)
{
	int err;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_BOND_REMOVAL)) {
		if (bonds_remove) {
			err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
			if (err) {
				LOG_ERR("Failed to remove bonds");
				goto error;
			} else {
				LOG_INF("Removed bonded devices");
			}
		}

		bonds_initialized = true;
	}

	err = ble_adv_start();
	if (err) {
		goto error;
	}

	return;
error:
	module_set_state(MODULE_STATE_ERROR);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state),
				MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);

			init();
			initialized = true;
		} else if (check_state(event, MODULE_ID(config), MODULE_STATE_READY)) {
			static bool started;

			__ASSERT_NO_MSG(!started);

			/* Settings need to be loaded before advertising start */
			start();
			started = true;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		if (is_ble_peer_event(eh)) {
			struct ble_peer_event *event = cast_ble_peer_event(eh);
			int err;

			switch (event->state) {
			case PEER_STATE_CONNECTED:
				k_delayed_work_cancel(&adv_update);
				k_delayed_work_cancel(&vendor_section_remove);
				break;

			case PEER_STATE_DISCONNECTED:
				err = ble_adv_start();
				if (err) {
					module_set_state(MODULE_STATE_ERROR);
				}
				break;

			default:
				/* Ignore */
				break;
			}

			return false;
		}

		if (is_power_down_event(eh)) {
			k_delayed_work_cancel(&adv_update);
			k_delayed_work_cancel(&vendor_section_remove);

			return false;
		}

		/* After wake up, device always starts in connected mode, no
		 * need to restart advertising with Swift Pair.
		 */
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_BOND_REMOVAL)) {
		if (is_button_event(eh)) {
			if (bonds_initialized) {
				return false;
			}

			const struct button_event *event =
				cast_button_event(eh);

			if ((event->key_id ==
			    CONFIG_DESKTOP_BLE_BOND_REMOVAL_BUTTON) &&
			    event->pressed) {
				bonds_remove = true;
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
#if CONFIG_DESKTOP_BLE_SWIFT_PAIR
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, power_event);
#endif /* CONFIG_DESKTOP_BLE_SWIFT_PAIR */
#if CONFIG_DESKTOP_BLE_BOND_REMOVAL
EVENT_SUBSCRIBE(MODULE, button_event);
#endif /* CONFIG_DESKTOP_BLE_BOND_REMOVAL */
