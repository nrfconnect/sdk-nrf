/* main.c - build-only exercise of the nRF71 SR coexistence driver API */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <drivers/nrf71_sr_coex/nrf71_sr_coex.h>
#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>

static int test_sr_enable(bool enable)
{
	ARG_UNUSED(enable);

	return 0;
}

static int test_sr_set_priority(const struct nrf71_sr_coex_sr_prio *prio)
{
	ARG_UNUSED(prio);

	return 0;
}

static const struct nrf71_sr_coex_sr_driver_cb test_sr_cb = {
	.enable = test_sr_enable,
	.set_priority = test_sr_set_priority,
};

/*
 * This test builds the standalone nRF71 SR coexistence driver together with the
 * nRF71 Wi-Fi driver and exercises the public API so the driver links against
 * the Wi-Fi FMAC coexistence transport.
 */
int main(void)
{
	const struct nrf71_sr_coex_user_params user = {
		.sr_rx_prot_ps_listen2inactive = 100,
		.sr_rx_prot_ps_inactive2listen = 100,
		.sr_rx_prot_calib_listen2inactive = 100,
		.sr_rx_prot_calib_inactive2listen = 100,
		.wifi_scan_prot = 100,
		.wifi_beacon_prot = 100,
		.wifi_conn_prot = 100,
		.wifi_calib_prot = 100,
		.ant_alloc = NRF71_SR_COEX_ANT_ALLOC_DYNAMIC,
	};
	const struct nrf71_sr_coex_cfg cfg = {
		.antenna = NRF71_SR_COEX_ANTENNA_SHARED,
		.user = &user,
	};

	(void)nrf71_sr_coex_init(&cfg);
	(void)nrf71_sr_coex_sr_register(&test_sr_cb);
	(void)nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_POWERED_UP_READY);
	(void)nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_PREPARE_POWER_DOWN);

	printk("nRF71 SR coexistence driver built\n");

	return 0;
}
