/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/nrf_cloud_log.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud.h>
#include "nrf_cloud_shadow.h"

LOG_MODULE_DECLARE(nrf_cloud_mqtt_device_message,
	CONFIG_NRF_CLOUD_MQTT_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

#define TEST_ACTIVE	"active"

/* Flag to indicate that the transform shadow data has been received */
static bool transform_rcvd;
/* Sample variable to demonstrate shadow configuration update */
static bool test_active;

int add_cfg_data(struct nrf_cloud_obj *const cfg_obj)
{
	/* Add the test active state */
	return nrf_cloud_obj_bool_add(cfg_obj, TEST_ACTIVE, test_active, false);
}

static int process_cfg(struct nrf_cloud_obj *const cfg_obj)
{
	bool val;
	int err;

	err = nrf_cloud_obj_bool_get(cfg_obj, TEST_ACTIVE, &val);
	if (err == 0) {
		LOG_INF("Setting test_active to: %s", val ? "true" : "false");
		test_active = val;
	} else if (err == -ENOMSG) {
		/* The key was found, but the value was not a boolean */
		LOG_WRN("Invalid configuration value");
		/* Reject the config */
		return -EBADF;
	}

	return err;
}

static int send_config(void)
{
	int err;

	NRF_CLOUD_OBJ_JSON_DEFINE(root_obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(state_obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(reported_obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(cfg_obj);

	if (nrf_cloud_obj_init(&cfg_obj) || nrf_cloud_obj_init(&reported_obj) ||
	    nrf_cloud_obj_init(&state_obj) || nrf_cloud_obj_init(&root_obj)) {
		err = -ENOMEM;
		goto cleanup;
	}

	/* Add test configuration */
	err = add_cfg_data(&cfg_obj);
	if (err) {
		goto cleanup;
	}

	/* Add config to reported */
	err = nrf_cloud_obj_object_add(&reported_obj, NRF_CLOUD_JSON_KEY_CFG, &cfg_obj, false);
	if (err) {
		goto cleanup;
	}

	/* Add reported to state */
	err = nrf_cloud_obj_object_add(&state_obj, NRF_CLOUD_JSON_KEY_REP, &reported_obj, false);
	if (err) {
		goto cleanup;
	}

	/* Add state to the root object */
	err = nrf_cloud_obj_object_add(&root_obj, NRF_CLOUD_JSON_KEY_STATE, &state_obj, false);
	if (err) {
		goto cleanup;
	}

	/* Send to the cloud */
	err = nrf_cloud_obj_shadow_update(&root_obj);

cleanup:
	nrf_cloud_obj_free(&root_obj);
	nrf_cloud_obj_free(&state_obj);
	nrf_cloud_obj_free(&reported_obj);
	nrf_cloud_obj_free(&cfg_obj);
	return err;
}

void shadow_config_cloud_connected(void)
{
	transform_rcvd = false;
}

int shadow_config_reported_send(void)
{
	LOG_INF("Sending shadow reported configuration");

	int err = send_config();

	if (err) {
		LOG_ERR("Failed to send configuration, error: %d", err);
	}

	return err;
}

static int shadow_config_delta_process(struct nrf_cloud_obj *const delta_obj)
{
	if (!delta_obj) {
		return -EINVAL;
	}

	if ((delta_obj->type != NRF_CLOUD_OBJ_TYPE_JSON) || !delta_obj->json) {
		/* No state JSON */
		return -ENOMSG;
	}

	/* If there is a pending delta event when the device establishes a cloud connection
	 * it is possible that it will be received before the accepted shadow data.
	 * Do not process a delta event until the accepted shadow data has been received.
	 * This is only a concern for MQTT.
	 */
	if (!transform_rcvd && IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		return -EAGAIN;
	}

	int err;

	NRF_CLOUD_OBJ_JSON_DEFINE(cfg_obj);

	/* Get the config object */
	err = nrf_cloud_obj_object_detach(delta_obj, NRF_CLOUD_JSON_KEY_CFG, &cfg_obj);
	if (err == -ENODEV) {
		/* No config in the delta */
		return -ENOMSG;
	}

	/* Process the configuration */
	err = process_cfg(&cfg_obj);

	if (err == -EBADF) {
		/* Clear incoming config and replace it with a good one */
		nrf_cloud_obj_free(&cfg_obj);
		nrf_cloud_obj_init(&cfg_obj);
		if (add_cfg_data(&cfg_obj)) {
			LOG_ERR("Failed to create delta response");
		}
	}

	/* Add the config object back into the state so the response can be sent */
	if (nrf_cloud_obj_object_add(delta_obj, NRF_CLOUD_JSON_KEY_CFG, &cfg_obj, false)) {
		nrf_cloud_obj_free(&cfg_obj);
		return -ENOMEM;
	}

	return err;
}

static int shadow_config_transform_process(struct nrf_cloud_obj *const transform_obj)
{
	if (!transform_obj) {
		return -EINVAL;
	}

	/* The transform shadow has been received */
	transform_rcvd = true;

	if ((transform_obj->type != NRF_CLOUD_OBJ_TYPE_JSON) || !transform_obj->json) {
		/* No config JSON */
		return -ENOMSG;
	}

	return 0;
}

/* Handler for nRF Cloud shadow events */
void handle_shadow_event(struct nrf_cloud_obj_shadow_data *const shadow)
{
	if (!shadow) {
		return;
	}

	int err;

	if ((shadow->type == NRF_CLOUD_OBJ_SHADOW_TYPE_DELTA) && shadow->delta) {
		LOG_DBG("Shadow: Delta");

		if (shadow->delta->is_err) {
			LOG_ERR("Shadow delta error code: %d, message: %s, ver: %d, ts: %lld",
				shadow->delta->error.code,
				shadow->delta->error.msg,
				shadow->delta->ver,
				shadow->delta->ts);
			return;
		}

		bool accept = true;

		err = shadow_config_delta_process(&shadow->delta->state);
		if (err == -EBADF) {
			LOG_INF("Rejecting shadow delta");
			accept = false;
		} else if (err == -ENOMEM) {
			LOG_ERR("Error handling shadow delta");
			return;
		} else if (err == -EAGAIN) {
			LOG_DBG("Ignoring delta until accepted shadow is received");
			return;
		}

		err = nrf_cloud_obj_shadow_delta_response_encode(&shadow->delta->state, accept);
		if (err) {
			LOG_ERR("Failed to encode shadow response: %d", err);
			return;
		}

		err = nrf_cloud_obj_shadow_update(&shadow->delta->state);
		if (err) {
			LOG_ERR("Failed to send shadow response, error: %d", err);
		}

	}  else if ((shadow->type == NRF_CLOUD_OBJ_SHADOW_TYPE_TF) && shadow->transform) {
		LOG_DBG("Shadow: Transform result");

		if (shadow->transform->is_err) {
			LOG_ERR("Shadow transform error: code %d, pos %d, msg %s",
				shadow->transform->error.code,
				shadow->transform->error.pos,
				shadow->transform->error.msg);
		} else {
			LOG_INF("Shadow transform result received");
			err = shadow_config_transform_process(&shadow->transform->result.obj);
			if (err) {
				/* Send the config on an error */
				(void)shadow_config_reported_send();
			}
		}
	} else {
		LOG_WRN("Shadow: Unrecognized shadow event");
	}
}
