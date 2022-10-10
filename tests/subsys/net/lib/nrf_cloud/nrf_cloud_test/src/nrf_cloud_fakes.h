/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_FAKES_H__
#define NRF_CLOUD_FAKES_H__

#include <nrf_cloud_transport.h>
#include <nrf_cloud_fsm.h>
#include <nrf_cloud_codec.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

/* Fake functions declaration */
FAKE_VALUE_FUNC(int, nct_init, const char *);
FAKE_VALUE_FUNC(int, nfsm_init);
FAKE_VALUE_FUNC(int, nrf_cloud_codec_init);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_fmfu_dev_set, const struct dfu_target_fmfu_fdev *);
FAKE_VALUE_FUNC(int, nct_disconnect);
FAKE_VOID_FUNC(nct_uninit);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_uninit);
FAKE_VALUE_FUNC(int, nct_connect);
FAKE_VALUE_FUNC(int, nct_socket_get);
FAKE_VALUE_FUNC(int, nct_keepalive_time_left);
FAKE_VALUE_FUNC(int, nct_process);

int fake_nct_init__succeeds(const char *const client_id)
{
	return 0;
}

int fake_nct_init__fails(const char *const cliend_id)
{
	return -ENODEV;
}

int fake_nfsm_init__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_codec_init__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_fota_fmfu_dev_set__succeeds(const struct dfu_target_fmfu_fdev *const dev_inf)
{
	return 0;
}

int fake_nrf_cloud_fota_fmfu_dev_set__fails(const struct dfu_target_fmfu_fdev *const dev_inf)
{
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

int fake_nct_keepalive_time_left_timeout(void)
{
	return -1;
}

int fake_nct_process__succeeds(void)
{
	return 0;
}

#endif /* NRF_CLOUD_FAKES_H__ */
