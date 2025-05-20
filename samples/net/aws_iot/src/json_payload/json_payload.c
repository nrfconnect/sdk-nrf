/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

#include "json_payload.h"

/* Register log module */
LOG_MODULE_REGISTER(json_payload, CONFIG_AWS_IOT_SAMPLE_LOG_LEVEL);

int json_payload_construct(char *message, size_t size, struct payload *payload)
{
	int err;
	const struct json_obj_descr parameters[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "uptime",
					  state.reported.uptime, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "app_version",
					  state.reported.app_version, JSON_TOK_STRING),
#if defined(CONFIG_MODEM_INFO)
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "modem_version",
					  state.reported.modem_version, JSON_TOK_STRING)
#endif
	};
	const struct json_obj_descr reported[] = {
		JSON_OBJ_DESCR_OBJECT_NAMED(struct payload, "reported", state.reported,
					    parameters),
	};
	const struct json_obj_descr root[] = {
		JSON_OBJ_DESCR_OBJECT(struct payload, state, reported),
	};

	err = json_obj_encode_buf(root, ARRAY_SIZE(root), payload, message, size);
	if (err) {
		LOG_ERR("json_obj_encode_buf, error: %d", err);
		return err;
	}

	return 0;
}
