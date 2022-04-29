/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <cJSON.h>

/* Kconfig Mocks */
#define CONFIG_LOCATION_METHODS_LIST_SIZE 3

/* date_time_now */
static int64_t date_time_now_value;
static bool date_time_now_fail;

void mocks_set_date_time_now(int64_t val)
{
	date_time_now_value = val;
	date_time_now_fail = false;
}

void mocks_set_date_time_now_fail(void)
{
	date_time_now_fail = true;
}

int mock_date_time_now(int64_t *unix_time_ms)
{
	*unix_time_ms = date_time_now_value;
	return date_time_now_fail ? (-EBUSY) : 0;
}

#define date_time_now mock_date_time_now


/* send_device_message_cJSON */
static cJSON *sent_device_message_obj;
static char *sent_device_message_str;
static bool sent_device_message;

void mocks_reset_sent_device_message(void)
{
	sent_device_message_str = NULL;
	sent_device_message_obj = NULL;
	sent_device_message = false;
}

void mocks_get_sent_device_message(cJSON **msg_obj, char **msg)
{
	*msg_obj = sent_device_message_obj;
	*msg = sent_device_message_str;
}

bool mocks_was_device_message_sent(void)
{
	return sent_device_message;
}

int mock_send_device_message_cJSON(cJSON *msg_obj)
{
	sent_device_message = true;
	if (msg_obj && cJSON_IsObject(msg_obj)) {
		sent_device_message_str = cJSON_PrintUnformatted(msg_obj);

		if (sent_device_message_str) {
			/* We re-parse the stringified msg_obj as a reliable
			 * (if inefficient) way to clone the original msg_obj
			 */
			sent_device_message_obj = cJSON_Parse(sent_device_message_str);
		} else {
			sent_device_message_obj = NULL;
			return -EBUSY;
		}
		return 0;
	}

	mocks_reset_sent_device_message();
	return -EBUSY;
}
#define send_device_message_cJSON mock_send_device_message_cJSON


/* cJSON */
static cJSON_Hooks bad_cjson_hooks;
static int cjson_malloc_fail_after;

static void *mock_malloc(size_t sz)
{
	if (cjson_malloc_fail_after == 0) {
		return NULL;
	}

	cjson_malloc_fail_after--;
	return k_malloc(sz);
}

static void mock_free(void *p_ptr)
{
	k_free(p_ptr);
}

/**
 * @brief init cJSON with a malloc mock designed to fail after a certain number of calls, and also
 * counts the total number of calls.
 *
 * @param fail_after -- How many calls should succeed before failure starts occurring.
 *			negative values will cause the mock to never fail.
 */
static void mocks_cJSON_Init_mocked(int fail_after)
{
	bad_cjson_hooks.malloc_fn = mock_malloc;
	bad_cjson_hooks.free_fn = mock_free;
	cJSON_InitHooks(&bad_cjson_hooks);
	cjson_malloc_fail_after = fail_after;
}
