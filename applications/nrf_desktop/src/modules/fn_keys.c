/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>

#define MODULE fn_keys
#include <caf/events/module_state_event.h>
#include <caf/events/button_event.h>

#include <caf/key_id.h>

#include "fn_key_id.h"
#include "fn_keys_def.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FN_KEYS_LOG_LEVEL);

#define FN_LOCK_STORAGE_NAME "fn_lock"

static bool fn_switch_active;
static bool fn_lock_active;

static uint16_t fn_key_pressed[CONFIG_DESKTOP_FN_KEYS_MAX_ACTIVE];
static size_t fn_key_pressed_count;


static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{

	if (!strcmp(key, FN_LOCK_STORAGE_NAME)) {
		ssize_t len = read_cb(cb_arg, &fn_lock_active,
				      sizeof(fn_lock_active));

		if ((len != sizeof(fn_lock_active)) || (len != len_rd)) {
			LOG_ERR("Can't read fn_lock_active from storage");

			return len;
		}
	}

	return 0;
}

#ifdef CONFIG_DESKTOP_STORE_FN_LOCK
SETTINGS_STATIC_HANDLER_DEFINE(fn_keys, MODULE_NAME, NULL, settings_set, NULL,
			       NULL);

#endif /* CONFIG_DESKTOP_STORE_FN_LOCK */


static void validate_enabled_fn_keys(void)
{
	static bool done;

	if (IS_ENABLED(CONFIG_ASSERT) && !done) {
		/* Validate the order of key IDs on the fn_keys array. */
		for (size_t i = 1; i < ARRAY_SIZE(fn_keys); i++) {
			if (fn_keys[i - 1] >= fn_keys[i]) {
				__ASSERT(false, "The fn_keys array must be "
						"sorted by key_id!");
			}
		}

		done = true;
	}
}

static void *bsearch(const void *key, const uint8_t *base,
		     size_t elem_num, size_t elem_size,
		     int (*compare)(const void *, const void *))
{
	__ASSERT_NO_MSG(base);
	__ASSERT_NO_MSG(compare);
	__ASSERT_NO_MSG(elem_num <= SSIZE_MAX);

	if (!elem_num) {
		return NULL;
	}

	ssize_t lower = 0;
	ssize_t upper = elem_num - 1;

	while (upper >= lower) {
		ssize_t m = (lower + upper) / 2;
		int cmp = compare(key, base + (elem_size * m));

		if (cmp == 0) {
			return (void *)(base + (elem_size * m));
		} else if (cmp < 0) {
			upper = m - 1;
		} else {
			lower = m + 1;
		}
	}

	/* key not found */
	return NULL;
}

static int key_id_compare(const void *a, const void *b)
{
	const uint16_t *p_a = a;
	const uint16_t *p_b = b;

	return (*p_a - *p_b);
}

static bool fn_key_enabled(uint16_t key_id)
{
	validate_enabled_fn_keys();

	uint16_t *p = bsearch(&key_id,
			   (uint8_t *)fn_keys,
			   ARRAY_SIZE(fn_keys),
			   sizeof(fn_keys[0]),
			   key_id_compare);

	return (p != NULL);
}

static void store_fn_lock(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_STORE_FN_LOCK)) {
		char key[] = MODULE_NAME "/" FN_LOCK_STORAGE_NAME;

		int err = settings_save_one(key, &fn_lock_active,
					    sizeof(fn_lock_active));

		if (err) {
			LOG_ERR("Problem storing fn_lock_active(err:%d)", err);
			module_set_state(MODULE_STATE_ERROR);
		}
	}
}

static bool button_event_handler(const struct button_event *event)
{
	if (unlikely(event->key_id == CONFIG_DESKTOP_FN_KEYS_SWITCH)) {
		fn_switch_active = event->pressed;
		return true;
	}

	if (unlikely(event->key_id == CONFIG_DESKTOP_FN_KEYS_LOCK)) {
		if (event->pressed) {
			fn_lock_active = !fn_lock_active;
			store_fn_lock();
		}
		return true;
	}

	bool fn_active = (fn_switch_active != fn_lock_active);

	if (unlikely(IS_FN_KEY(event->key_id))) {
		return false;
	}

	if (likely(!fn_key_enabled(event->key_id))) {
		return false;
	}

	if (event->pressed && fn_active) {
		if (fn_key_pressed_count == ARRAY_SIZE(fn_key_pressed)) {
			LOG_WRN("No space to handle fn key remapping.");
			return false;
		}

		fn_key_pressed[fn_key_pressed_count] = event->key_id;
		fn_key_pressed_count++;

		struct button_event *new_event = new_button_event();
		new_event->pressed = true;
		new_event->key_id = FN_KEY_ID(KEY_COL(event->key_id),
					      KEY_ROW(event->key_id));
		APP_EVENT_SUBMIT(new_event);

		return true;
	}

	if (!event->pressed) {
		bool key_was_remapped = false;

		for (size_t i = 0; i < fn_key_pressed_count; i++) {
			if (key_was_remapped) {
				fn_key_pressed[i - 1] = fn_key_pressed[i];
			} else if (fn_key_pressed[i] == event->key_id) {
				key_was_remapped = true;
			} else {
				/* skip */
			}
		}

		if (key_was_remapped) {
			fn_key_pressed_count--;

			struct button_event *new_event = new_button_event();
			new_event->pressed = false;
			new_event->key_id = FN_KEY_ID(KEY_COL(event->key_id),
						      KEY_ROW(event->key_id));
			APP_EVENT_SUBMIT(new_event);

			return true;
		}
	}

	return false;
}

static void silence_unused(void)
{
	/* These things will be opt-out by the compiler. */
	if (!IS_ENABLED(CONFIG_DESKTOP_STORE_FN_LOCK)) {
		ARG_UNUSED(settings_set);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_button_event(aeh)) {
		return button_event_handler(cast_button_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			silence_unused();
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, button_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
