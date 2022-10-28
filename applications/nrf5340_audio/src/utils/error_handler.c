/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log_ctrl.h>

#include "board.h"
#include "led.h"

/* Print everything from the error handler */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(error_handler, CONFIG_ERROR_HANDLER_LOG_LEVEL);

static void led_error_indication(void)
{
#if defined(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)
	(void)led_on(LED_APP_RGB, LED_COLOR_RED);
#elif defined(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUNET)
	(void)led_on(LED_NET_RGB, LED_COLOR_RED);
#endif /* defined(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP) */
}

void error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
#if (CONFIG_DEBUG)
	LOG_ERR("Caught system error -- reason %d. Entering infinite loop", reason);
	LOG_PANIC();
	led_error_indication();
	irq_lock();
	while (true) {
		__asm__ volatile("nop");
	}

#else
	LOG_ERR("Caught system error -- reason %d. Cold rebooting.", reason);
#if (CONFIG_LOG)
	LOG_PANIC();
#endif /* (CONFIG_LOG) */
	led_error_indication();
	sys_reboot(SYS_REBOOT_COLD);
#endif /* (CONFIG_DEBUG) */
	CODE_UNREACHABLE;
}

void bt_ctlr_assert_handle(char *c, int code)
{
	LOG_ERR("BT Controller assert: %s, code: 0x%x", c, code);
	error_handler(code, NULL);
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
	error_handler(reason, esf);
}

void assert_post_action(const char *file, unsigned int line)
{
	LOG_ERR("Assert post action: file: %s, line %d", file, line);
	error_handler(0, NULL);
}
