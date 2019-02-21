/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>

#include <zephyr.h>
#include <settings/settings.h>

#include "event_manager.h"
#include "config_event.h"

#define MODULE config
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CONFIG_LOG_LEVEL);

#define KEY_LEN 9

/* Array of pointers to data loaded from settings */
static struct config_event *loaded_data[CONFIG_EVENT_ID_MAX];

static bool config_id_is_supported(u8_t id)
{
	if (IS_ENABLED(CONFIG_DESKTOP_MOTION_OPTICAL_ENABLE)) {
		if ((id >= CONFIG_EVENT_ID_MOTION_CPI) &&
		    (id <= CONFIG_EVENT_ID_MOTION_DOWNSHIFT_REST2)) {
			return true;
		}
	}

	return false;
}

static int settings_set(int argc, char **argv, void *val_ctx)
{
	if (argc != 1) {
		return -ENOENT;
	}

	char *end;
	long int val = strtol(argv[0], &end, 10);
	if ((*end != '\0') || (val < 0) || (val > UCHAR_MAX)) {
		LOG_WRN("ID is not a valid number");
		return 0;
	}

	u8_t id = (u8_t)val;

	if (!config_id_is_supported(id)) {
		LOG_WRN("ID %u not supported", id);
		return 0;
	}

	if (!loaded_data[id]) {
		loaded_data[id] = new_config_event();
		loaded_data[id]->id = id;
		/* Do not store nor forward loaded config to other devices */
		loaded_data[id]->recipient = 0;
	}

	int len = settings_val_read_cb(val_ctx, loaded_data[id]->data,
				       sizeof(loaded_data[id]->data));
	if (len != sizeof(loaded_data[id]->data)) {
		LOG_ERR("Can't read config (id:%u err:%d)", id, len);
		k_free(loaded_data[id]);
		loaded_data[id] = NULL;
		return len;
	}

	return 0;
}

static int commit(void)
{
	/* All settings loaded, including FCB duplicates.
	 * We can apply values to application.
	 */
	for (u8_t id = 0; id < CONFIG_EVENT_ID_MAX; id++) {
		if (!loaded_data[id]) {
			continue;
		}

		EVENT_SUBMIT(loaded_data[id]);
		loaded_data[id] = NULL;
	}

	return 0;
}

static void update_config(const u8_t config_id, u8_t *data, size_t size)
{
	char key[KEY_LEN];
	int err = snprintk(key, sizeof(key), MODULE_NAME "/%u", config_id);
	__ASSERT_NO_MSG(err < sizeof(key));

	if (err < 0) {
		LOG_ERR("Creating key failed");
	} else {
		LOG_INF("Store %s", log_strdup(key));
		err = settings_save_one(key, data, size);
		if (err) {
			LOG_ERR("Cannot store err %d", err);
		}
	}
}

static int enable_settings(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		static struct settings_handler sh = {
			.name = MODULE_NAME,
			.h_set = settings_set,
			.h_commit = commit,
		};

		err = settings_register(&sh);
		if (err) {
			LOG_ERR("Cannot register settings handler");
			goto error;
		}
	}

	/* This module loads settings for all application modules */
	err = settings_load();
	if (err) {
		LOG_ERR("Cannot load settings");
		goto error;
	}

	LOG_INF("Settings loaded");

error:
	return err;
}

static void init(void)
{
	int err = enable_settings();

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}

static bool module_event_handler(const struct module_state_event *event)
{
	/* Settings need to be loaded after all client modules are ready. */
	const void * const req_modules[] = {
		MODULE_ID(main),
#if CONFIG_DESKTOP_HIDS_ENABLE
		MODULE_ID(hids),
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
		MODULE_ID(bas),
#endif
#if CONFIG_DESKTOP_BLE_ADVERTISING_ENABLE
		MODULE_ID(ble_adv),
#endif
#if CONFIG_DESKTOP_MOTION_OPTICAL_ENABLE
		MODULE_ID(motion),
#endif
#if CONFIG_DESKTOP_BLE_SCANNING_ENABLE
		MODULE_ID(ble_scan),
#endif
	};

	static u32_t req_state;

	static_assert(ARRAY_SIZE(req_modules) < (8 * sizeof(req_state)),
		      "Array size bigger than number of bits");

	if (req_state == BIT_MASK(ARRAY_SIZE(req_modules))) {
		/* Already initialized */
		return false;
	}

	for (size_t i = 0; i < ARRAY_SIZE(req_modules); i++) {
		if (check_state(event, req_modules[i], MODULE_STATE_READY)) {
			unsigned int flag = BIT(i);

			/* Catch double initialization of any module */
			__ASSERT_NO_MSG((req_state & flag) == 0);

			req_state |= flag;

			if (req_state == BIT_MASK(ARRAY_SIZE(req_modules))) {
				init();
			}

			break;
		}
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(eh)) {
			struct config_event *event = cast_config_event(eh);

			/* Do not store events addressed to other devices.
			 * Accept only events coming from transport. Do not
			 * write already stored information.
			 */
			if (event->recipient == CONFIG_USB_DEVICE_PID) {
				update_config(event->id, event->data,
					      sizeof(event->data));
			}

			return false;
		}
	}

	if (is_module_state_event(eh)) {
		return module_event_handler(cast_module_state_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_event);
#endif
