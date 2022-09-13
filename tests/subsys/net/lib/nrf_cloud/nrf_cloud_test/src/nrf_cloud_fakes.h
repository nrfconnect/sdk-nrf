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

#endif /* NRF_CLOUD_FAKES_H__ */
