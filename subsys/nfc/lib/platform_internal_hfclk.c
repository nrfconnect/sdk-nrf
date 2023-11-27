/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <nrfx_nfct.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "platform_internal.h"

static struct onoff_manager *hf_mgr;
static struct onoff_client cli;

static void clock_handler(struct onoff_manager *mgr, int res)
{
	/* Activate NFCT only when HFXO is running */
	nrfx_nfct_state_force(NRFX_NFCT_STATE_ACTIVATED);
}

int nfc_platform_internal_hfclk_init(void)
{
	hf_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	if (hf_mgr) {
		return 0;
	} else {
		return -EIO;
	}
}

int nfc_platform_internal_hfclk_start(void)
{
	sys_notify_init_callback(&cli.notify, clock_handler);
	return onoff_request(hf_mgr, &cli);
}

int nfc_platform_internal_hfclk_stop(void)
{
	return onoff_cancel_or_release(hf_mgr, &cli);
}
