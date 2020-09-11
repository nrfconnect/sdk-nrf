/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <device.h>
#include <coap_server_client_interface.h>
#include <net/coap_utils.h>
#include <logging/log.h>
#include <net/openthread.h>
#include <net/socket.h>
#include <openthread/thread.h>

#include "coap_client_utils.h"

LOG_MODULE_REGISTER(coap_client_utils, CONFIG_COAP_CLIENT_UTILS_LOG_LEVEL);

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))
#define RESPONSE_POLL_PERIOD 100

static uint32_t poll_period;

static bool is_connected;

static struct k_work unicast_light_work;
static struct k_work multicast_light_work;
static struct k_work toggle_MTD_SED_work;
static struct k_work provisioning_work;
static struct k_work on_connect_work;
static struct k_work on_disconnect_work;

mtd_mode_toggle_cb_t on_mtd_mode_toggle;

/* Options supported by the server */
static const char *const light_option[] = { LIGHT_URI_PATH, NULL };
static const char *const provisioning_option[] = { PROVISIONING_URI_PATH,
						   NULL };

/* Thread multicast mesh local address */
static struct sockaddr_in6 multicast_local_addr = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(COAP_PORT),
	.sin6_addr.s6_addr = { 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
	.sin6_scope_id = 0U
};

/* Variable for storing server address acquiring in provisioning handshake */
static char unique_local_addr_str[INET6_ADDRSTRLEN];
static struct sockaddr_in6 unique_local_addr = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(COAP_PORT),
	.sin6_addr.s6_addr = {0, },
	.sin6_scope_id = 0U
};

static bool is_mtd_in_med_mode(otInstance *instance)
{
	return otThreadGetLinkMode(instance).mRxOnWhenIdle;
}

static void poll_period_response_set(void)
{
	otError error;

	otInstance *instance = openthread_get_default_instance();

	if (is_mtd_in_med_mode(instance)) {
		return;
	}

	if (!poll_period) {
		poll_period = otLinkGetPollPeriod(instance);

		error = otLinkSetPollPeriod(instance, RESPONSE_POLL_PERIOD);
		__ASSERT(error == OT_ERROR_NONE, "Failed to set pool period");

		LOG_INF("Poll Period: %dms set", RESPONSE_POLL_PERIOD);
	}
}

static void poll_period_restore(void)
{
	otError error;
	otInstance *instance = openthread_get_default_instance();

	if (is_mtd_in_med_mode(instance)) {
		return;
	}

	if (poll_period) {
		error = otLinkSetPollPeriod(instance, poll_period);
		__ASSERT_NO_MSG(error == OT_ERROR_NONE);

		LOG_INF("Poll Period: %dms restored", poll_period);
		poll_period = 0;
	}
}

static int on_provisioning_reply(const struct coap_packet *response,
				 struct coap_reply *reply,
				 const struct sockaddr *from)
{
	int ret = 0;
	const uint8_t *payload;
	uint16_t payload_size = 0u;

	ARG_UNUSED(reply);
	ARG_UNUSED(from);

	payload = coap_packet_get_payload(response, &payload_size);

	if (payload_size != sizeof(unique_local_addr.sin6_addr)) {
		LOG_ERR("Received data size is invalid");
	}

	memcpy(&unique_local_addr.sin6_addr, payload, payload_size);

	if (!inet_ntop(AF_INET6, payload, unique_local_addr_str,
		       INET6_ADDRSTRLEN)) {
		LOG_ERR("Received data is not IPv6 address: %d", errno);
		ret = -errno;
		goto exit;
	}

	LOG_INF("Received peer address: %s", log_strdup(unique_local_addr_str));

exit:
	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		poll_period_restore();
	}

	return ret;
}

static void toggle_one_light(struct k_work *item)
{
	uint8_t payload = (uint8_t)THREAD_COAP_UTILS_LIGHT_CMD_TOGGLE;

	ARG_UNUSED(item);

	if (unique_local_addr.sin6_addr.s6_addr16[0] == 0) {
		LOG_WRN("Peer address not set. Activate 'provisioning' option "
			"on the server side");
		return;
	}

	LOG_INF("Send 'light' request to: %s",
		log_strdup(unique_local_addr_str));
	coap_send_request(COAP_METHOD_PUT,
			  (const struct sockaddr *)&unique_local_addr,
			  light_option, &payload, sizeof(payload), NULL);
}

static void toggle_mesh_lights(struct k_work *item)
{
	static uint8_t command = (uint8_t)THREAD_COAP_UTILS_LIGHT_CMD_OFF;

	ARG_UNUSED(item);

	command = ((command == THREAD_COAP_UTILS_LIGHT_CMD_OFF) ?
			   THREAD_COAP_UTILS_LIGHT_CMD_ON :
			   THREAD_COAP_UTILS_LIGHT_CMD_OFF);

	LOG_INF("Send multicast mesh 'light' request");
	coap_send_request(COAP_METHOD_PUT,
			  (const struct sockaddr *)&multicast_local_addr,
			  light_option, &command, sizeof(command), NULL);
}

static void send_provisioning_request(struct k_work *item)
{
	ARG_UNUSED(item);

	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		/* decrease the polling period for higher responsiveness */
		poll_period_response_set();
	}

	LOG_INF("Send 'provisioning' request");
	coap_send_request(COAP_METHOD_GET,
			  (const struct sockaddr *)&multicast_local_addr,
			  provisioning_option, NULL, 0u, on_provisioning_reply);
}

static void toggle_minimal_sleepy_end_device(struct k_work *item)
{
	otError error;
	struct otInstance *instance = openthread_get_default_instance();
	otLinkModeConfig mode = otThreadGetLinkMode(instance);

#if IS_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT)
	struct device *cons = device_get_binding(CONSOLE_LABEL);
#endif

	if (mode.mRxOnWhenIdle) {
		mode.mRxOnWhenIdle = false;

#if IS_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT)
		device_set_power_state(cons, DEVICE_PM_OFF_STATE,
				       NULL, NULL);
#endif
	} else {
		mode.mRxOnWhenIdle = true;

#if IS_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT)
		device_set_power_state(cons, DEVICE_PM_ACTIVE_STATE,
				       NULL, NULL);
#endif
	}

	error = otThreadSetLinkMode(instance, mode);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set MLE link mode configuration");
		return;
	}

	on_mtd_mode_toggle(mode.mRxOnWhenIdle);
}

static void on_thread_state_changed(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			k_work_submit(&on_connect_work);
			is_connected = true;
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
		default:
			k_work_submit(&on_disconnect_work);
			is_connected = false;
			break;
		}
	}
}

static void submit_work_if_connected(struct k_work *work)
{
	if (is_connected) {
		k_work_submit(work);
	} else {
		LOG_INF("Connection is broken");
	}
}

void coap_client_utils_init(ot_connection_cb_t on_connect,
			    ot_disconnection_cb_t on_disconnect,
			    mtd_mode_toggle_cb_t on_toggle)
{
	on_mtd_mode_toggle = on_toggle;

	coap_init(AF_INET6, NULL);

	k_work_init(&on_connect_work, on_connect);
	k_work_init(&on_disconnect_work, on_disconnect);
	k_work_init(&unicast_light_work, toggle_one_light);
	k_work_init(&multicast_light_work, toggle_mesh_lights);
	k_work_init(&provisioning_work, send_provisioning_request);

	openthread_set_state_changed_cb(on_thread_state_changed);
	openthread_start(openthread_get_default_context());

	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		k_work_init(&toggle_MTD_SED_work,
			    toggle_minimal_sleepy_end_device);
		k_work_submit(&toggle_MTD_SED_work);
	}
}

void coap_client_toggle_one_light(void)
{
	submit_work_if_connected(&unicast_light_work);
}

void coap_client_toggle_mesh_lights(void)
{
	submit_work_if_connected(&multicast_light_work);
}

void coap_client_send_provisioning_request(void)
{
	submit_work_if_connected(&provisioning_work);
}

void coap_client_toggle_minimal_sleepy_end_device(void)
{
	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		k_work_submit(&toggle_MTD_SED_work);
	}
}
