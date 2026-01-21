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

/* Flag to indicate that the accepted shadow data has been received */
static bool accepted_rcvd;

void send_initial_log_level(void)
{
	int err;

	NRF_CLOUD_OBJ_JSON_DEFINE(root_obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(ctrl_obj);

	err = nrf_cloud_obj_init(&root_obj);
	if (err) {
		LOG_ERR("Failed to initialize root object: %d", err);
		goto cleanup;
	}

	err = nrf_cloud_obj_init(&ctrl_obj);
	if (err) {
		LOG_ERR("Failed to initialize config object: %d", err);
		(void)nrf_cloud_obj_free(&root_obj);
		goto cleanup;
	}

	/* Create the config object */
	err = nrf_cloud_obj_num_add(&ctrl_obj, NRF_CLOUD_JSON_KEY_LOG,
		(double)nrf_cloud_log_control_get(), false);
	if (err) {
		LOG_ERR("Failed to add reported log level, error: %d", err);
		goto cleanup;
	}

	/* Add the config object to the root object */
	err = nrf_cloud_obj_object_add(&root_obj, NRF_CLOUD_JSON_KEY_CTRL, &ctrl_obj, false);
	if (err) {
		LOG_ERR("Failed to add config object to root object: %d", err);
		goto cleanup;
	}

	/* Send the initial config */
	err = nrf_cloud_obj_shadow_update(&root_obj);
	if (err) {
		LOG_ERR("Failed to send initial config, error: %d", err);
	} else {
		LOG_INF("Initial config sent");
	}
cleanup:
	(void)nrf_cloud_obj_free(&ctrl_obj);
	(void)nrf_cloud_obj_free(&root_obj);
}

void shadow_config_cloud_connected(void)
{
	accepted_rcvd = false;
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
	if (!accepted_rcvd && IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
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

	if (err == -EBADF) {
		/* Clear incoming config */
		nrf_cloud_obj_free(&cfg_obj);
	}

	/* Add the config object back into the state so the response can be sent */
	if (nrf_cloud_obj_object_add(delta_obj, NRF_CLOUD_JSON_KEY_CFG, &cfg_obj, false)) {
		nrf_cloud_obj_free(&cfg_obj);
		return -ENOMEM;
	}

	return err;
}

static int shadow_config_accepted_process(struct nrf_cloud_obj *const accepted_obj)
{
	if (!accepted_obj) {
		return -EINVAL;
	}

	/* The accepted shadow has been received */
	accepted_rcvd = true;

	if ((accepted_obj->type != NRF_CLOUD_OBJ_TYPE_JSON) || !accepted_obj->json) {
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
		LOG_DBG("Shadow: Delta - version: %d, timestamp: %lld",
			shadow->delta->ver,
			shadow->delta->ts);

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

	} else if ((shadow->type == NRF_CLOUD_OBJ_SHADOW_TYPE_ACCEPTED) && shadow->accepted) {
		LOG_DBG("Shadow: Accepted");
		err = shadow_config_accepted_process(&shadow->accepted->config);
		if (err) {
			/* Send the config on an error */
			send_initial_log_level();
		}
	}
}
