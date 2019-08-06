/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "protocol_timer.h"

struct k_poll_signal nfc_tnep_sig_timer;

struct nfc_tnep_timer_data {
	size_t t_wait;
	u8_t n, n_max;
} static nfc_tnep_timer_service_data;

static void nfc_tnep_timer_twait(struct k_timer *timer);

K_TIMER_DEFINE(nfc_tnep_timer, nfc_tnep_timer_twait, NULL);

static void nfc_tnep_timer_twait(struct k_timer *timer)
{
	struct nfc_tnep_timer_data *data;

	data = (struct nfc_tnep_timer_data *) k_timer_user_data_get(timer);

	k_poll_signal_raise(&nfc_tnep_sig_timer,
			    NFC_TNEP_TMER_SIGNAL_TIMER_PERIOD);

	data->n++;

	if (data->n >= data->n_max) {
		k_timer_stop(&nfc_tnep_timer);
		nfc_tnep_timer_service_data.n = 0;

		k_poll_signal_raise(&nfc_tnep_sig_timer,
				    NFC_TNEP_TMER_SIGNAL_TIMER_STOP);
	}
}

void nfc_tnep_timer_init(size_t period_ms, u8_t counts)
{
	k_timer_user_data_set(&nfc_tnep_timer, &nfc_tnep_timer_service_data);

	nfc_tnep_timer_service_data.t_wait = period_ms;
	nfc_tnep_timer_service_data.n_max = counts;
	nfc_tnep_timer_service_data.n = 0;
}

void nfc_tnep_timer_start(void)
{
	nfc_tnep_timer_stop();

	k_timer_stop(&nfc_tnep_timer);

	k_timer_start(&nfc_tnep_timer,
	NFC_TNEP_K_TIMER_START_DELAY,
		      K_MSEC(nfc_tnep_timer_service_data.t_wait));
}

void nfc_tnep_timer_stop(void)
{
	k_timer_stop(&nfc_tnep_timer);

	nfc_tnep_timer_service_data.n = 0;
}
