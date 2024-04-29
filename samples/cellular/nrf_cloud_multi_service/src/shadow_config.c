/* Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/nrf_cloud_defs.h>

#include "shadow_config.h"
#if defined(CONFIG_NRF_CLOUD_COAP)
#include "shadow_support_coap.h"
#endif
#include "application.h"

LOG_MODULE_REGISTER(shadow_config, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#define TEST_COUNTER_EN	"counterEnable"

/* Flag to indicate that the accepted shadow data has been received (MQTT only) */
static bool accepted_rcvd;

static int add_cfg_data(struct nrf_cloud_obj *const cfg_obj)
{
	/* Add the test counter state */
	return nrf_cloud_obj_bool_add(cfg_obj, TEST_COUNTER_EN, test_counter_enable_get(), false);
}

static int process_cfg(struct nrf_cloud_obj *const cfg_obj)
{
	bool val;
	int err = nrf_cloud_obj_bool_get(cfg_obj, TEST_COUNTER_EN, &val);

	if (err == 0) {
		/* The expected key/value was found, set the test counter enable state */
		test_counter_enable_set(val);
	} else if (err == -ENOMSG) {
		/* The key was found, but the value was not a boolean */
		LOG_WRN("Invalid configuration value");
		/* Reject the config */
		err = -EBADF;
	} else {
		LOG_DBG("Expected data not found in config object");
		err = -ENOMSG;
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

	/* Add the supported configuration data */
	err = add_cfg_data(&cfg_obj);
	if (err) {
		goto cleanup;
	}

#if defined(CONFIG_NRF_CLOUD_MQTT)

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
#else  /* CONFIG_NRF_CLOUD_COAP */

	/* Add config to root */
	err = nrf_cloud_obj_object_add(&root_obj, NRF_CLOUD_JSON_KEY_CFG, &cfg_obj, false);
	if (err) {
		goto cleanup;
	}

	err = shadow_support_coap_obj_send(&root_obj, false);
#endif

cleanup:
	nrf_cloud_obj_free(&root_obj);
	nrf_cloud_obj_free(&state_obj);
	nrf_cloud_obj_free(&reported_obj);
	nrf_cloud_obj_free(&cfg_obj);
	return err;
}

void shadow_config_cloud_connected(void)
{
	accepted_rcvd = false;
}

int shadow_config_reported_send(void)
{
	LOG_INF("Sending reported configuration");

	int err = send_config();

	if (err) {
		LOG_ERR("Failed to send configuration, error: %d", err);
	}

	return err;
}

int shadow_config_delta_process(struct nrf_cloud_obj *const delta_obj)
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

int shadow_config_accepted_process(struct nrf_cloud_obj *const accepted_obj)
{
	if (!accepted_obj) {
		return -EINVAL;
	}

	if ((accepted_obj->type != NRF_CLOUD_OBJ_TYPE_JSON) || !accepted_obj->json) {
		/* No config JSON */
		return -ENOMSG;
	}

	int err = process_cfg(accepted_obj);

	if (err == 0) {
		accepted_rcvd = true;
	}

	return err;
}
