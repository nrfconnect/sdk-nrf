/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE smp
#include <caf/events/module_state_event.h>
#include <caf/events/ble_smp_event.h>

#include <zephyr/mgmt/mcumgr/smp_bt.h>
#include <img_mgmt/img_mgmt.h>
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include <os_mgmt/os_mgmt.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_BLE_SMP_LOG_LEVEL);

static atomic_t event_active = ATOMIC_INIT(false);


static int upload_confirm(const struct img_mgmt_upload_req req,
			  const struct img_mgmt_upload_action action)
{
	if (atomic_cas(&event_active, false, true)) {
		APP_EVENT_SUBMIT(new_ble_smp_transfer_event());
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ble_smp_transfer_event(aeh)) {
		bool res = atomic_cas(&event_active, true, false);

		__ASSERT_NO_MSG(res);
		ARG_UNUSED(res);

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			img_mgmt_set_upload_cb(upload_confirm);
			img_mgmt_register_group();
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
			os_mgmt_register_group();
#endif
		} else if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			int err = smp_bt_register();

			if (err) {
				LOG_ERR("Service init failed (err %d)", err);
				module_set_state(MODULE_STATE_ERROR);
			} else {
				LOG_INF("Service initialized");
				LOG_INF("MCUboot image version: %s", CONFIG_MCUBOOT_IMAGE_VERSION);
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_smp_transfer_event);
