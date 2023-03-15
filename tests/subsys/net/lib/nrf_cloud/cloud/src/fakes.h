/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_cloud_transport.h>
#include <nrf_cloud_codec_internal.h>
#include <nrf_cloud_mem.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>

DEFINE_FFF_GLOBALS;

int poll(struct zsock_pollfd *fds, int nfds, int timeout);

/* Fake functions declaration */
FAKE_VALUE_FUNC(int, nct_init, const char *);
FAKE_VALUE_FUNC(int, nfsm_init);
FAKE_VALUE_FUNC(int, nrf_cloud_codec_init, struct nrf_cloud_os_mem_hooks *);
FAKE_VOID_FUNC(nrf_cloud_set_app_version, const char *const);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_fmfu_dev_set, const struct dfu_target_fmfu_fdev *);
FAKE_VALUE_FUNC(int, nct_disconnect);
FAKE_VOID_FUNC(nct_uninit);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_uninit);
FAKE_VALUE_FUNC(int, nct_connect);
FAKE_VALUE_FUNC(int, nct_socket_get);
FAKE_VALUE_FUNC(int, nct_keepalive_time_left);
FAKE_VALUE_FUNC(int, nct_process);
FAKE_VALUE_FUNC(int, nct_cc_send, const struct nct_cc_data *);
FAKE_VALUE_FUNC(int, nct_dc_send, const struct nct_dc_data *);
FAKE_VALUE_FUNC(int, nct_dc_stream, const struct nct_dc_data *);
FAKE_VALUE_FUNC(int, nct_dc_bulk_send, const struct nct_dc_data *, enum mqtt_qos);
FAKE_VALUE_FUNC(int, nrf_cloud_shadow_dev_status_encode,
				const struct nrf_cloud_device_status *,
				struct nrf_cloud_data *, const bool);
FAKE_VOID_FUNC(nrf_cloud_device_status_free, struct nrf_cloud_data *);
FAKE_VALUE_FUNC(int, nrf_cloud_sensor_data_encode, const struct nrf_cloud_sensor_data *,
				struct nrf_cloud_data *);
FAKE_VOID_FUNC(nrf_cloud_os_mem_hooks_init, struct nrf_cloud_os_mem_hooks *);
FAKE_VOID_FUNC(nrf_cloud_free, void *);
FAKE_VALUE_FUNC(int, poll, struct zsock_pollfd *, int, int);

/* Custom fakes implementation */
int fake_nct_init__succeeds(const char *const client_id)
{
	ARG_UNUSED(client_id);
	return 0;
}

int fake_nct_init__fails(const char *const client_id)
{
	ARG_UNUSED(client_id);
	return -ENODEV;
}

int fake_nfsm_init__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_codec_init__succeeds(struct nrf_cloud_os_mem_hooks *hooks)
{
	ARG_UNUSED(hooks);
	return 0;
}

int fake_nrf_cloud_fota_fmfu_dev_set__succeeds(const struct dfu_target_fmfu_fdev *const dev_inf)
{
	ARG_UNUSED(dev_inf);
	return 0;
}

int fake_nrf_cloud_fota_fmfu_dev_set__fails(const struct dfu_target_fmfu_fdev *const dev_inf)
{
	ARG_UNUSED(dev_inf);
	return -ENODEV;
}

int fake_nct_disconnect__succeeds(void)
{
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED,
	};

	nfsm_set_current_state_and_notify(STATE_INITIALIZED, &evt);
	return 0;
}

int fake_nct_disconnect__not_actually_disconnect(void)
{
	return 0;
}

int fake_nct_disconnect__fails(void)
{
	return -ENOMEM;
}

int fake_nrf_cloud_fota_uninit__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_fota_uninit__busy(void)
{
	return -EBUSY;
}

int fake_nct_connect__succeeds(void)
{
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTED,
	};

	nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);
	return 0;
}

int fake_nct_connect_with_cc_connect__succeeds(void)
{
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTED,
	};

	nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);

	/* Sleep for 1s */
	k_sleep(K_MSEC(1000));

	evt.type = NRF_CLOUD_EVT_RX_DATA_GENERAL;
	nfsm_set_current_state_and_notify(STATE_CC_CONNECTED, &evt);
	return 0;
}

int fake_nct_connect_with_dc_connect__succeeds(void)
{
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTED,
	};

	nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);

	/* Sleep for 1s */
	k_sleep(K_MSEC(1000));

	evt.type = NRF_CLOUD_EVT_READY;
	nfsm_set_current_state_and_notify(STATE_DC_CONNECTED, &evt);
	return 0;
}

int fake_nct_connect__invalid(void)
{
	return -EINVAL;
}

int fake_nct_socket_get_data(void)
{
	return 1234;
}

int fake_nct_keepalive_time_left_get(void)
{
	return 1000;
}

int fake_nct_keepalive_time_left_long_timeout(void)
{
	return 11000;
}

int fake_nct_process__succeeds(void)
{
	return 0;
}

int fake_nct_process__fails(void)
{
	return -EACCES;
}

int fake_nct_cc_send__succeeds(const struct nct_cc_data *data)
{
	ARG_UNUSED(data);
	return 0;
}

int fake_nct_cc_send__fails(const struct nct_cc_data *data)
{
	ARG_UNUSED(data);
	return -EINVAL;
}

int fake_nct_dc_send__succeeds(const struct nct_dc_data *data)
{
	ARG_UNUSED(data);
	return 0;
}

int fake_nct_dc_send__fails(const struct nct_dc_data *data)
{
	ARG_UNUSED(data);
	return -EINVAL;
}

int fake_nct_dc_stream__succeeds(const struct nct_dc_data *data)
{
	ARG_UNUSED(data);
	return 0;
}

int fake_nct_dc_stream__fails(const struct nct_dc_data *data)
{
	ARG_UNUSED(data);
	return -EINVAL;
}

int fake_nct_dc_bulk_send__succeeds(const struct nct_dc_data *data, enum mqtt_qos qos)
{
	ARG_UNUSED(data);
	ARG_UNUSED(qos);
	return 0;
}

int fake_nct_dc_bulk_send__fails(const struct nct_dc_data *data, enum mqtt_qos qos)
{
	ARG_UNUSED(data);
	ARG_UNUSED(qos);
	return -EINVAL;
}

int fake_device_status_shadow_encode__succeeds(
	const struct nrf_cloud_device_status * const dev_status,
	struct nrf_cloud_data * const output, const bool include_state)
{
	ARG_UNUSED(dev_status);
	ARG_UNUSED(output);
	ARG_UNUSED(include_state);
	return 0;
}

int fake_device_status_shadow_encode__fails(
	const struct nrf_cloud_device_status * const dev_status,
	struct nrf_cloud_data * const output, const bool include_state)
{
	ARG_UNUSED(dev_status);
	ARG_UNUSED(output);
	ARG_UNUSED(include_state);
	return -EINVAL;
}

int fake_nrf_cloud_sensor_data_encode__fails(
	const struct nrf_cloud_sensor_data *sensor,
	struct nrf_cloud_data *output)
{
	ARG_UNUSED(sensor);
	ARG_UNUSED(output);
	return -ENOMEM;
}

int fake_nrf_cloud_sensor_data_encode__succeeds(
	const struct nrf_cloud_sensor_data *sensor,
	struct nrf_cloud_data *output)
{
	ARG_UNUSED(sensor);
	output->ptr = NULL;
	return 0;
}

/* Declarations of functions and variables for nrf_cloud_run */
#define pollfd zsock_pollfd
#define POLLIN ZSOCK_POLLIN
#define POLLERR ZSOCK_POLLERR
#define POLLHUP ZSOCK_POLLHUP
#define POLLNVAL ZSOCK_POLLNVAL

int fake_poll__zero(struct zsock_pollfd *fds, int nfds, int timeout)
{
	ARG_UNUSED(fds);
	ARG_UNUSED(nfds);
	k_sleep(K_MSEC(timeout));
	return 0;
}

int fake_poll__negative(struct zsock_pollfd *fds, int nfds, int timeout)
{
	ARG_UNUSED(fds);
	ARG_UNUSED(nfds);
	k_sleep(K_MSEC(timeout));
	return -1;
}

int fake_poll__pollin(struct zsock_pollfd *fds, int nfds, int timeout)
{
	k_sleep(K_MSEC(timeout));
	fds[0].revents = POLLIN;
	return nfds;
}

int fake_poll__pollnval(struct zsock_pollfd *fds, int nfds, int timeout)
{
	k_sleep(K_MSEC(timeout));
	fds[0].revents = POLLNVAL;
	return nfds;
}

int fake_poll__pollhup(struct zsock_pollfd *fds, int nfds, int timeout)
{
	k_sleep(K_MSEC(timeout));
	fds[0].revents = POLLHUP;
	return nfds;
}

int fake_poll__pollerr(struct zsock_pollfd *fds, int nfds, int timeout)
{
	k_sleep(K_MSEC(timeout));
	fds[0].revents = POLLERR;
	return nfds;
}
