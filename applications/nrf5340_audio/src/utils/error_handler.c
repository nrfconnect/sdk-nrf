/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/gpio.h>

/* Print everything from the error handler */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(error_handler, CONFIG_ERROR_HANDLER_LOG_LEVEL);

#if (CONFIG_DEBUG)
struct error_led_setting {
	struct gpio_dt_spec gpio_spec;
	gpio_flags_t status;
};

#if defined(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)
/* nRF5340 Audio DK center RGB LED */
static const struct error_led_setting error_leds[] = {
	{GPIO_DT_SPEC_GET(DT_NODELABEL(rgb1_red), gpios), GPIO_OUTPUT_ACTIVE},
	{GPIO_DT_SPEC_GET(DT_NODELABEL(rgb1_green), gpios), GPIO_OUTPUT_INACTIVE},
	{GPIO_DT_SPEC_GET(DT_NODELABEL(rgb1_blue), gpios), GPIO_OUTPUT_INACTIVE}};
#else
static const struct error_led_setting error_leds[] = {
	{GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios), GPIO_OUTPUT_ACTIVE},
	{GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios), GPIO_OUTPUT_ACTIVE},
	{GPIO_DT_SPEC_GET(DT_NODELABEL(led2), gpios), GPIO_OUTPUT_ACTIVE},
	{GPIO_DT_SPEC_GET(DT_NODELABEL(led3), gpios), GPIO_OUTPUT_ACTIVE}};
#endif /* CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP */
#endif /* CONFIG_DEBUG */

void error_handler(unsigned int reason, const struct arch_esf *esf)
{
#if (CONFIG_DEBUG)
	LOG_ERR("Caught system error -- reason %d. Entering infinite loop", reason);
	LOG_PANIC();

	for (int i = 0; i < ARRAY_SIZE(error_leds); i++) {
		(void)gpio_pin_configure_dt(&error_leds[i].gpio_spec, error_leds[i].status);
	}

	irq_lock();
	while (1) {
		__asm__ volatile("nop");
	}
#else
	LOG_ERR("Caught system error -- reason %d. Cold rebooting.", reason);
#if (CONFIG_LOG)
	LOG_PANIC();
#endif /* (CONFIG_LOG) */
	sys_reboot(SYS_REBOOT_COLD);
#endif /* (CONFIG_DEBUG) */

	CODE_UNREACHABLE;
}

__weak void bt_ctlr_assert_handle(char *c, int code)
{
	LOG_ERR("BT controller assert: %s, code: 0x%x", c, code);
	error_handler(code, NULL);
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	error_handler(reason, esf);
}

void assert_post_action(const char *file, unsigned int line)
{
	LOG_ERR("Assert post action: file: %s, line %d", file, line);
	error_handler(0, NULL);
}
