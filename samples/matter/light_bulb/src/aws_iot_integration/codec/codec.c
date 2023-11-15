/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

#include "codec.h"

/* Register log module */
LOG_MODULE_REGISTER(codec, CONFIG_AWS_IOT_INTEGRATION_LOG_LEVEL);

int codec_json_encode_update_message(char *message, size_t size, struct payload *payload)
{
	int err;
	const struct json_obj_descr node[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "onoff", state.reported.node.onoff.onoff,
					  JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "level_control", state.reported.node.onoff.level_control,
					  JSON_TOK_NUMBER)
	};
	const struct json_obj_descr reported[] = {
		JSON_OBJ_DESCR_OBJECT_NAMED(struct payload, "reported", state.reported, node),
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

int codec_json_decode_delta_message(char *message, size_t size, struct payload *payload)
{
	int err;
	const struct json_obj_descr attributes[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "onoff",
					  state.reported.node.onoff.onoff,
					  JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "level_control",
					  state.reported.node.onoff.level_control,
					  JSON_TOK_NUMBER)
	};
	const struct json_obj_descr root[] = {
		JSON_OBJ_DESCR_OBJECT(struct payload, state, attributes),
	};

	err = json_obj_parse(message, size, root, ARRAY_SIZE(root), payload);
	if (err != 1) {
		LOG_ERR("json_obj_parse, error: %d", err);
		return err;
	}

	return 0;
}
