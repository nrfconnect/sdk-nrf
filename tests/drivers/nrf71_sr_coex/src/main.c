/* main.c - build-only exercise of the nRF71 SR coexistence driver API */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <common/fw_if/nrf71_coex_if.h>
#include <common/fw_if/nrf71_cd_sr_if.h>

/*
 * This test builds the standalone nRF71 SR coexistence driver together with the
 * nRF71 Wi-Fi driver and exercises the coex_cd_* API contract so the driver
 * links against the Wi-Fi FMAC coexistence transport and the Short-Range driver
 * stubs.
 */
int main(void)
{
	(void)coex_cd_sr_power_notify(COEX_SR_POWERED_UP_READY);
	(void)coex_cd_sr_power_notify(COEX_SR_PREPARE_POWER_DOWN);

	printk("nRF71 SR coexistence driver built\n");

	return 0;
}
