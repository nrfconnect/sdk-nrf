/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include <mgmt/smp_bt.h>

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif

#define MODULE smp
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_SMP_LOG_LEVEL);


static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			if (IS_ENABLED(CONFIG_MCUMGR_CMD_IMG_MGMT)) {
				img_mgmt_register_group();
			}
		} else if (check_state(event, MODULE_ID(ble_state),
					      MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			int err = smp_bt_register();
			if (err) {
				LOG_ERR("service init failed");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				LOG_INF("service initialized");
				module_set_state(MODULE_STATE_READY);
			}
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
