/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bulk_commands, CONFIG_BRIDGE_BULK_LOG_LEVEL);

#define ID_DAP_VENDOR14		       (0x80 + 14)
#define ID_DAP_VENDOR15		       (0x80 + 15)
#define ID_DAP_VENDOR_NRF53_BOOTLOADER ID_DAP_VENDOR14
#define ID_DAP_VENDOR_NRF91_BOOTLOADER ID_DAP_VENDOR15

static const struct gpio_dt_spec reset_pin =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), reset_gpios, {});
static const struct gpio_dt_spec button1_pin =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), button1_gpios, {});

static void nrf91_bootloader_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(nrf91_bootloader_work, nrf91_bootloader_work_handler);

static void nrf91_bootloader_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	if (reset_pin.port && device_is_ready(reset_pin.port)) {
		LOG_DBG("setting reset pin");
		gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT_LOW);
	}
	if (button1_pin.port && device_is_ready(button1_pin.port)) {
		LOG_DBG("setting button pin");
		gpio_pin_configure_dt(&button1_pin, GPIO_OUTPUT_LOW);
	}
	k_sleep(K_MSEC(100));
	if (reset_pin.port && device_is_ready(reset_pin.port)) {
		gpio_pin_configure_dt(&reset_pin, GPIO_DISCONNECTED);
	}
	k_sleep(K_MSEC(1000));
	if (button1_pin.port && device_is_ready(button1_pin.port)) {
		gpio_pin_configure_dt(&button1_pin, GPIO_DISCONNECTED);
	}
}

/* The is a placeholder implementation until proper CMSIS-DAP support is available. */
size_t dap_execute_cmd(uint8_t *in, uint8_t *out)
{
	LOG_DBG("got command 0x%02X", in[0]);

#ifdef CONFIG_RETENTION_BOOT_MODE
	if (in[0] == ID_DAP_VENDOR_NRF53_BOOTLOADER) {
		bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
		sys_reboot(SYS_REBOOT_WARM);
	}
#endif /* CONFIG_RETENTION_BOOT_MODE */
	if (in[0] == ID_DAP_VENDOR_NRF91_BOOTLOADER) {
		if (!k_work_busy_get(&nrf91_bootloader_work.work)) {
			k_work_reschedule(&nrf91_bootloader_work, K_NO_WAIT);
		}
		out[0] = in[0];
		out[1] = 0x00;
		return 2;
	}

	/* default reply: command failed */
	out[0] = in[0];
	out[1] = 0xFF;
	return 2;
}
