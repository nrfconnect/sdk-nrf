/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_fota.h>
#include <lwm2m_resource_ids.h>
#include <lwm2m_rd_client.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <net/socket.h>

#include "cloud/cloud_wrapper.h"

#define MODULE lwm2m_integration

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define LWM2M_INTEGRATION_CLIENT_ID_LEN 15
#else
#define LWM2M_INTEGRATION_CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif

#if defined(CONFIG_LWM2M_INTEGRATION)
BUILD_ASSERT(!IS_ENABLED(CONFIG_NRF_CLOUD_PGPS),
	     "P-GPS is not supported when building for LwM2M");
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

/* Register callbacks for nrf_modem_lib_init. */
NRF_MODEM_LIB_ON_INIT(lwm2m_integration_init_hook, on_modem_init, NULL);

/* Function that is called on nrf_modem_lib_init. Used during verification of new firwmare images
 * after FOTA (reboot).
 */
static void on_modem_init(int err, void *ctx)
{
	int ret = nrf_modem_lib_get_init_ret();
	struct update_counter counter = { 0 };

	/* Modem DFU requires that the application is rebooted.
	 * This is handled in the application module.
	 */
	if (ret == MODEM_DFU_RESULT_OK) {
		ret = fota_update_counter_read(&counter);
		if (ret) {
			LOG_ERR("Failed read the update counter, err: %d", ret);
			return;
		}

		if (counter.update != -1) {
			ret = fota_update_counter_update(COUNTER_CURRENT,
							 counter.update);
			if (ret) {
				LOG_ERR("Failed to update the update counter, err: %d",
					ret);
			}
		}
	}
}

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
	char pathstr[MAX_RESOURCE_LEN];
	int err, len;
	uint32_t current_lifetime = 0;
	uint32_t lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;

	len = snprintk(pathstr, sizeof(pathstr), "1/%d/1", srv_obj_inst);
	if ((len < 0) || (len >= sizeof(pathstr))) {
		return;
	}

	err = lwm2m_engine_get_u32(pathstr, &current_lifetime);
	if (err) {
		LOG_ERR("Failed getting current session lifetime, error: %d", err);
		return;
	}

	if (current_lifetime != lifetime) {
		err = lwm2m_engine_set_u32(pathstr, lifetime);
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
		state = DISCONNECTED;
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
		state = DISCONNECTED;
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
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
		LOG_WRN("LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		state = DISCONNECTED;
		break;
	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE");
		break;
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_WRN("LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		state = DISCONNECTED;
		break;
	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_DISCONNECT");
		break;
	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_DBG("LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF");
		break;
	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_ERR("LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		state = DISCONNECTED;
		break;
	default:
		LOG_ERR("Unknown event: %d", client_event);
		break;
	}

	if (notify) {
		cloud_wrapper_notify_event(&cloud_wrap_evt);
	}
}

/* Callback handler triggered when lwm2m object resource 1/0/4 (device/reboot) is executed. */
static int device_reboot_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);
	ARG_UNUSED(obj_inst_id);

	struct cloud_wrap_event cloud_wrap_evt = {
		.type = CLOUD_WRAP_EVT_REBOOT_REQUEST
	};

	cloud_wrapper_notify_event(&cloud_wrap_evt);
	return 0;
}

int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler)
{
	int err, len;

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
	char imei_buf[20 + sizeof("OK\r\n")];

	/* Retrieve device IMEI from modem. */
	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Not able to retrieve device IMEI from modem");
		return err;
	}

	/* Set null character at the end of the device IMEI. */
	imei_buf[LWM2M_INTEGRATION_CLIENT_ID_LEN] = 0;

	strncpy(client_id_buf, imei_buf, sizeof(client_id_buf) - 1);
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

	LOG_DBG("LwM2M endpoint name: %s", log_strdup(endpoint_name));

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

	err = lwm2m_init_security(&client, endpoint_name, NULL);
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
		char buf[1 + sizeof(CONFIG_LWM2M_INTEGRATION_PSK) / 2];
		size_t len = hex2bin(CONFIG_LWM2M_INTEGRATION_PSK,
				     sizeof(CONFIG_LWM2M_INTEGRATION_PSK) - 1, buf,
				     sizeof(buf));

		err = lwm2m_engine_set_opaque("0/0/5", buf, len);
		if (err) {
			LOG_ERR("Failed setting PSK, error: %d", err);
			return err;
		}
	}
#endif /* CONFIG_LWM2M_INTEGRATION_PROVISION_CREDENTIALS */

	err = lwm2m_init_image();
	if (err < 0) {
		LOG_ERR("lwm2m_init_image, error: %d", err);
		return err;
	}

	/* Register callback for reboot requests. */
	err = lwm2m_engine_register_exec_callback(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
							     DEVICE_OBJECT_REBOOT_RID),
						  device_reboot_cb);
	if (err) {
		LOG_ERR("lwm2m_engine_register_exec_callback, error: %d", err);
		return err;
	}

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

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id, char *path_list[])
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	int err;

	err = lwm2m_engine_send(&client, (const char **)path_list, len, ack);
	if (err) {
		LOG_ERR("lwm2m_engine_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id)
{
	return -ENOTSUP;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id, char *path_list[])
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	int err;

	err = lwm2m_engine_send(&client, (const char **)path_list, len, ack);
	if (err) {
		LOG_ERR("lwm2m_engine_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_neighbor_cells_send(char *buf, size_t len, bool ack, uint32_t id, char *path_list[])
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	int err;

	err = lwm2m_engine_send(&client, (const char **)path_list, len, ack);
	if (err) {
		LOG_ERR("lwm2m_engine_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_agps_request_send(char *buf, size_t len, bool ack, uint32_t id, char *path_list[])
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(id);

	int err;

	err = lwm2m_engine_send(&client, (const char **)path_list, len, ack);
	if (err) {
		LOG_ERR("lwm2m_engine_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	return -ENOTSUP;
}

int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	return -ENOTSUP;
}
