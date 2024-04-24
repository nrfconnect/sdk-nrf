/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_memconf.h>
LOG_MODULE_REGISTER(app);

int main(void)
{

#if defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUPPR)
		csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL,
			VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_HIBERNATE);
#endif

	int cnt = 0;

	while (1) {
		LOG_INF("test %d", cnt++);
		printk("Hello world from %s\n", CONFIG_BOARD);
		k_msleep(1000);
	}

	return 0;
}
