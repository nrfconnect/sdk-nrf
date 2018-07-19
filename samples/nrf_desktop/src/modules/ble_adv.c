#include <zephyr/types.h>

#include <misc/reboot.h>

#include <bluetooth/bluetooth.h>

#define MODULE ble_adv
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BLE_ADV_LEVEL
#include <logging/sys_log.h>


#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
#if CONFIG_NRF_BT_HIDS
			  0x12, 0x18,	/* HID Service */
#endif
			  0x0f, 0x18),	/* Battery Service */
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void ble_adv_start(void)
{
	/* TODO: use bond manager to check if it possible to pair with another
	 * device. Currently bt_keys and bt_settings APIs are private
	 */
	int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));

	if (err) {
		SYS_LOG_ERR("Advertising failed to start (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	SYS_LOG_INF("Advertising started");

	module_set_state(MODULE_STATE_READY);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const void * const required_srv[] = {
#if CONFIG_NRF_BT_HIDS
			MODULE_ID(hids),
#endif
			MODULE_ID(bas),
			MODULE_ID(dis),
		};
		static unsigned int srv_ready_cnt;

		struct module_state_event *event = cast_module_state_event(eh);

		SYS_LOG_DBG("event from %s", event->name);

		for (size_t i = 0; i < ARRAY_SIZE(required_srv); i++) {
			if (check_state(event, required_srv[i], MODULE_STATE_READY)) {
				srv_ready_cnt++;
				SYS_LOG_DBG("received %s ready! cnt: %u",
					    required_srv[i], srv_ready_cnt);

				if (srv_ready_cnt == ARRAY_SIZE(required_srv)) {
					static bool initialized;

					__ASSERT_NO_MSG(!initialized);
					initialized = true;

					ble_adv_start();
				}
				break;
			}
		}
		__ASSERT_NO_MSG(srv_ready_cnt <= ARRAY_SIZE(required_srv));

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
