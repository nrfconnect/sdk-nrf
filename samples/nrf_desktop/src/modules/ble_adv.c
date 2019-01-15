/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <misc/reboot.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>

#define MODULE ble_adv
#include "module_state_event.h"
#include "button_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_ADV_LOG_LEVEL);


#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
#if CONFIG_BT_GATT_HIDS
			  0x12, 0x18,	/* HID Service */
#endif
			  0x0f, 0x18),	/* Battery Service */
#if CONFIG_DESKTOP_BLE_SWIFT_PAIR
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
			  0x06, 0x00,	/* Microsoft Vendor ID */
			  0x03,		/* Microsoft Beacon ID */
			  0x00,		/* Microsoft Beacon Sub Scenario */
			  0x80),	/* Reserved RSSI Byte */
#endif
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct k_delayed_work adv_param_update;
static struct bt_le_adv_param adv_param = {
	.options = BT_LE_ADV_OPT_CONNECTABLE,
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
};

static bool bonds_initialized;
static bool bonds_remove;

static void ble_adv_param_update_fn(struct k_work *work)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SWIFT_PAIR)) {
		LOG_INF("Switch to normal cadence advertising");
		adv_param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
		adv_param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

		err = bt_le_adv_stop();

		if (!err) {
			bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
					sd, ARRAY_SIZE(sd));
		}

		if (err) {
			LOG_ERR("Failed to restart advertising (err %d)", err);
			sys_reboot(SYS_REBOOT_WARM);
		}

		vendor_section_remove = true;
		k_delayed_work_submit(&adv_update,
			K_SECONDS(SWIFT_PAIR_SECTION_REMOVE_TIMEOUT));
	}
}
#endif /* CONFIG_DESKTOP_BLE_SWIFT_PAIR */

static void ble_adv_start(void)
{
#if CONFIG_DESKTOP_BLE_SWIFT_PAIR
	k_delayed_work_init(&adv_update, ble_adv_update_fn);

	LOG_INF("Swift pair enabled, use fast advertising");
	adv_param.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	adv_param.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
#endif

	int err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	LOG_INF("Advertising started");

#if CONFIG_DESKTOP_BLE_SWIFT_PAIR
	k_delayed_work_submit(&adv_update,
		K_SECONDS(CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT));
#endif

	module_set_state(MODULE_STATE_READY);
}

static void ble_settings_load(void)
{
	/* Settings need to be loaded after GATT services are setup, otherwise
	 * the values stored in flash will not be written to GATT database.
	 */
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		if (settings_load()) {
			LOG_ERR("Cannot load settings");
			sys_reboot(SYS_REBOOT_WARM);
		}
		LOG_INF("Settings loaded");

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_BOND_REMOVAL)) {
			if (bonds_remove) {
				if (bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY)) {
					LOG_ERR("Failed to remove bonds");
				} else {
					LOG_WRN("Removed bonded devices");
				}
			}

			bonds_initialized = true;
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const void * const required_srv[] = {
#if CONFIG_BT_GATT_HIDS
			MODULE_ID(hids),
#endif
			MODULE_ID(bas),
		};
		static unsigned int srv_ready_cnt;

		struct module_state_event *event = cast_module_state_event(eh);

		for (size_t i = 0; i < ARRAY_SIZE(required_srv); i++) {
			if (check_state(event, required_srv[i], MODULE_STATE_READY)) {
				srv_ready_cnt++;

				if (srv_ready_cnt == ARRAY_SIZE(required_srv)) {
					static bool initialized;

					__ASSERT_NO_MSG(!initialized);
					initialized = true;

					ble_settings_load();
					ble_adv_start();
				}
				break;
			}
		}
		__ASSERT_NO_MSG(srv_ready_cnt <= ARRAY_SIZE(required_srv));

		return false;
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
#if CONFIG_DESKTOP_BLE_BOND_REMOVAL
EVENT_SUBSCRIBE(MODULE, button_event);
#endif /* CONFIG_DESKTOP_BLE_BOND_REMOVAL */
