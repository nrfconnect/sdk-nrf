/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>
#include <lwm2m_resource_ids.h>
#include <lwm2m_rd_client.h>
#include <nrf_modem_at.h>
#include <zephyr/net/socket.h>
#include <hw_id.h>

#include "cloud/cloud_wrapper.h"

#define MODULE lwm2m_integration

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define LWM2M_INTEGRATION_CLIENT_ID_LEN 15
#else
#define LWM2M_INTEGRATION_CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif

/* Resource ID used to register a reboot callback. */
#define DEVICE_OBJECT_REBOOT_RID 4
#define MAX_RESOURCE_LEN 20

/* Internal states. */
static enum lwm2m_integration_state_type {
	DISCONNECTED,
	CONNECTING,
	CONNECTED,
} state;

static cloud_wrap_evt_handler_t wrapper_evt_handler;

/* LWM2M client instance. */
static struct lwm2m_ctx client;

static char client_id_buf[LWM2M_INTEGRATION_CLIENT_ID_LEN + 1];
static char endpoint_name[sizeof(CONFIG_LWM2M_INTEGRATION_ENDPOINT_PREFIX) +
			  LWM2M_INTEGRATION_CLIENT_ID_LEN];

/* Enable session lifetime check after initial boot. After bootstrapping, the bootstrap server
 * will override the configured lifetime CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME.
 * Therefore we check the session lifetime and overrides it with the configured value if
 * it differs.
 */
static bool update_session_lifetime = true;

void client_acknowledge(void)
{
	lwm2m_acknowledge(&client);
}

static void cloud_wrapper_notify_event(const struct cloud_wrap_event *evt)
{
	if ((wrapper_evt_handler != NULL) && (evt != NULL)) {
		wrapper_evt_handler(evt);
	} else {
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

/* Function used to override the configured session lifetime,
 * CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME.
 */
static void rd_client_update_lifetime(int srv_obj_inst)
{
	int err;
	uint32_t current_lifetime = 0;
	uint32_t lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	struct lwm2m_obj_path path = LWM2M_OBJ(1, srv_obj_inst, 1);

	err = lwm2m_get_u32(&path, &current_lifetime);
	if (err) {
		LOG_ERR("Failed getting current session lifetime, error: %d", err);
		return;
	}

	if (current_lifetime != lifetime) {
		err = lwm2m_set_u32(&path, lifetime);
		if (err) {
			LOG_ERR("Failed setting current session lifetime, error: %d", err);
			return;
		}

		LOG_DBG("Update session lifetime from %d to %d", current_lifetime, lifetime);
	}

	update_session_lifetime = false;
}

static void rd_client_event(struct lwm2m_ctx *client, enum lwm2m_rd_client_event client_event)
{
	ARG_UNUSED(client);

	struct cloud_wrap_event cloud_wrap_evt = { 0 };
	bool notify = false;

	switch (client_event) {
	case LWM2M_RD_CLIENT_EVENT_NONE:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_NONE");
		break;
	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		LOG_WRN("LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		break;
	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE");

		/* Trigger update of session lifetime after bootstrap. */
		update_session_lifetime = true;

		/* Bootstrap registration complete. Lwm2m engine will proceed to connect to
		 * the management server.
		 */
		state = CONNECTING;
		break;
	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE");
		break;
	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_WRN("LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		break;
	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE");

		if (update_session_lifetime) {
			/* Read and update the current server lifetime value. */
			rd_client_update_lifetime(client->srv_obj_inst);
		}

		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTED;
		notify = true;
		state = CONNECTED;
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT:
		LOG_WRN("LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTING;
		state = CONNECTING;
		notify = true;
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE");
		if (state == CONNECTING) {
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTED;
			notify = true;
			state = CONNECTED;
		}
		break;
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_WRN("LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		notify = true;
		break;
	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_DISCONNECT");
		break;
	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF");
		break;
	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_ERR("LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		notify = true;
		break;
	default:
		LOG_ERR("Unknown event: %d", client_event);
		break;
	}

	/* If a LwM2M failure has occurred, we explicitly stop the engine before the cloud module
	 * is notified with the CLOUD_WRAP_EVT_DISCONNECTED event. This is to clear up any
	 * LwM2M engine state to ensure that we are able to perform a clean restart of the engine.
	 */
	if (notify && cloud_wrap_evt.type == CLOUD_WRAP_EVT_DISCONNECTED) {
		int err = cloud_wrap_disconnect();

		if (err) {
			LOG_ERR("cloud_wrap_disconnect, error: %d", err);
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		}
	}

	if (notify) {
		cloud_wrapper_notify_event(&cloud_wrap_evt);
	}
}

/* Callback handler triggered when lwm2m object resource 1/0/4 (device/reboot) is executed. */
static int device_reboot_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	ARG_UNUSED(obj_inst_id);

	struct cloud_wrap_event cloud_wrap_evt = { .type = CLOUD_WRAP_EVT_REBOOT_REQUEST };

	if (args_len && args && *args == REBOOT_SOURCE_FOTA_OBJ) {
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_DONE;
	}

	cloud_wrapper_notify_event(&cloud_wrap_evt);
	return 0;
}

/* Callback handler triggered when the modem should be put in a certain functional mode.
 * Handler is called pre provisioning of DTLS credentials when the modem should be put in
 * offline mode, and when the modem should return to normal mode after
 * provisioning has been carried out.
 */
static int modem_mode_request_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	ARG_UNUSED(user_data);

	int err;
	enum lte_lc_func_mode mode_current;
	struct cloud_wrap_event cloud_wrap_evt = { 0 };

	err = lte_lc_func_mode_get(&mode_current);
	if (err) {
		LOG_ERR("lte_lc_func_mode_get failed, error: %d", err);
		return err;
	}

	/* Return success if the modem is in the required functional mode. */
	if (mode_current == new_mode) {
		return 0;
	}

	switch (new_mode) {
	case LTE_LC_FUNC_MODE_OFFLINE:
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_LTE_DISCONNECT_REQUEST;
		break;
	case LTE_LC_FUNC_MODE_NORMAL:
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_LTE_CONNECT_REQUEST;
		break;
	default:
		LOG_ERR("Non supported modem functional mode request.");
		return -ENOTSUP;
	}

	cloud_wrapper_notify_event(&cloud_wrap_evt);

	/* If the modem is not in the required functional mode,
	 * return the time that the security object should wait before the handler is called again.
	 * Set by CONFIG_LWM2M_INTEGRATION_MODEM_MODE_REQUEST_RETRY_SECONDS.
	 */
	return CONFIG_LWM2M_INTEGRATION_MODEM_MODE_REQUEST_RETRY_SECONDS;
}

static int firmware_update_state_cb(uint8_t update_state)
{
	int err;
	uint8_t update_result;
	struct cloud_wrap_event cloud_wrap_evt = { 0 };

	/* Get the firmware object update result code */
	err = lwm2m_get_u8(&LWM2M_OBJ(5, 0, 5), &update_result);
	if (err) {
		LOG_ERR("Failed getting firmware result resource value");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		cloud_wrap_evt.err = err;
		cloud_wrapper_notify_event(&cloud_wrap_evt);
		return 0;
	}

	switch (update_state) {
	case STATE_IDLE:
		LOG_DBG("STATE_IDLE, result: %d", update_result);

		/* If the FOTA state returns to its base state STATE_IDLE, the FOTA failed. */
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_ERROR;
		break;
	case STATE_DOWNLOADING:
		LOG_DBG("STATE_DOWNLOADING, result: %d", update_result);
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_START;
		break;
	case STATE_DOWNLOADED:
		LOG_DBG("STATE_DOWNLOADED, result: %d", update_result);
		return 0;
	case STATE_UPDATING:
		LOG_DBG("STATE_UPDATING, result: %d", update_result);
		/* Disable further callbacks from FOTA */
		lwm2m_firmware_set_update_state_cb(NULL);
		return 0;
	default:
		LOG_ERR("Unknown state: %d", update_state);
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_ERROR;
		break;
	}

	cloud_wrapper_notify_event(&cloud_wrap_evt);
	return 0;
}

int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler)
{
	int err, len;
	struct modem_mode_change mode_change = {
		.cb = modem_mode_request_cb,
		.user_data = NULL
	};

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));

	if (err) {
		LOG_ERR("Failed to retrieve device ID");
		return err;
	}

	strncpy(client_id_buf, hw_id_buf, sizeof(client_id_buf) - 1);

	/* Explicitly null terminate client_id_buf to be sure that we carry a
	 * null terminated buffer after strncpy().
	 */
	client_id_buf[sizeof(client_id_buf) - 1] = '\0';
#else
	len = snprintk(client_id_buf, sizeof(client_id_buf), "%s", CONFIG_CLOUD_CLIENT_ID);
	if ((len < 0) || (len >= sizeof(endpoint_name))) {
		return -ERANGE;
	}
#endif

	len = snprintk(endpoint_name, sizeof(endpoint_name), "%s%s",
		       CONFIG_LWM2M_INTEGRATION_ENDPOINT_PREFIX, client_id_buf);
	if ((len < 0) || (len >= sizeof(endpoint_name))) {
		return -ERANGE;
	}

	LOG_DBG("LwM2M endpoint name: %s", endpoint_name);

	if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION_FLUSH_SESSION_CACHE)) {
		int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_DTLS_1_2);

		if (fd < 0) {
			LOG_ERR("Failed to create socket: %d", errno);
			return -errno;
		}

		int cache_flush = 1U;

		err = setsockopt(fd, SOL_TLS, TLS_SESSION_CACHE_PURGE,
				 &cache_flush, sizeof(cache_flush));
		if (err < 0) {
			LOG_ERR("Failed to set flush session cache: %d", -errno);
		}

		err = close(fd);
		if (err) {
			LOG_ERR("Failed to close socket: %d", errno);
			return -errno;
		}

		LOG_DBG("DTLS session cache flushed.");
	}

	err = lwm2m_init_security(&client, endpoint_name, &mode_change);
	if (err) {
		LOG_ERR("lwm2m_init_security, error: %d", err);
		return err;
	}

	err = lwm2m_init_firmware();
	if (err) {
		LOG_ERR("lwm2m_init_firmware, error: %d", err);
		return err;
	}

	/* If provided, the PSK used to connect to the bootstrap/management server is provisioned
	 * to the modem at run-time. In general this method is discouraged and the PSK should be
	 * provisoned to the modem before running the application.
	 */
#if defined(CONFIG_LWM2M_INTEGRATION_PROVISION_CREDENTIALS)
	if (IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) && sizeof(CONFIG_LWM2M_INTEGRATION_PSK) > 1) {
		if (!IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) ||
		    (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) &&
		     lwm2m_security_needs_bootstrap())) {
			char buf[1 + sizeof(CONFIG_LWM2M_INTEGRATION_PSK) / 2];
			size_t len =
				hex2bin(CONFIG_LWM2M_INTEGRATION_PSK,
					sizeof(CONFIG_LWM2M_INTEGRATION_PSK) - 1, buf, sizeof(buf));

			err = lwm2m_set_opaque(&LWM2M_OBJ(0, 0, 5), buf, len);
			if (err) {
				LOG_ERR("Failed setting PSK, error: %d", err);
				return err;
			}
		}
	}
#endif /* CONFIG_LWM2M_INTEGRATION_PROVISION_CREDENTIALS */

	err = lwm2m_init_image();
	if (err < 0) {
		LOG_ERR("lwm2m_init_image, error: %d", err);
		return err;
	}

	/* Register callback for reboot requests. */
	err = lwm2m_register_exec_callback(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0,
						      DEVICE_OBJECT_REBOOT_RID),
					   device_reboot_cb);
	if (err) {
		LOG_ERR("lwm2m_register_exec_callback, error: %d", err);
		return err;
	}

	lwm2m_firmware_set_update_state_cb(firmware_update_state_cb);

	wrapper_evt_handler = event_handler;
	state = DISCONNECTED;
	return 0;
}

int cloud_wrap_connect(void)
{
	int err;

	if (state != DISCONNECTED) {
		return -EINPROGRESS;
	}

	err = lwm2m_rd_client_start(
			&client, endpoint_name,
			lwm2m_security_needs_bootstrap() ? LWM2M_RD_CLIENT_FLAG_BOOTSTRAP : 0,
			rd_client_event, NULL);
	if (err) {
		LOG_ERR("lwm2m_rd_client_start, error: %d", err);
		return err;
	}

	state = CONNECTING;
	return 0;
}

int cloud_wrap_disconnect(void)
{
	int err;

	if (state != CONNECTED) {
		return -ENOTSUP;
	}

	err = lwm2m_rd_client_stop(&client, rd_client_event, false);
	if (err) {
		LOG_ERR("lwm2m_rd_client_stop, error: %d", err);
		return err;
	}

	state = DISCONNECTED;
	return 0;
}

int cloud_wrap_state_get(bool ack, uint32_t id)
{
	return -ENOTSUP;
}

int cloud_wrap_state_send(char *buf, size_t len, bool ack, uint32_t id)
{
	return -ENOTSUP;
}

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id,
			 const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	int err;

	err = lwm2m_send(&client, path_list, len, ack);
	if (err) {
		LOG_ERR("lwm2m_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id)
{
	return -ENOTSUP;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id,
		       const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	int err;

	err = lwm2m_send(&client, path_list, len, ack);
	if (err) {
		LOG_ERR("lwm2m_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_cloud_location_send(char *buf, size_t len, bool ack, uint32_t id)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	return location_assistance_ground_fix_request_send(&client, ack);
}

int cloud_wrap_agps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	return location_assistance_agps_request_send(&client, ack);
}

int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	return location_assistance_pgps_request_send(&client, ack);
}

int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	return -ENOTSUP;
}
