/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/gpio.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bulk_commands, CONFIG_BRIDGE_BULK_LOG_LEVEL);

#define ID_DAP_VENDOR14			(0x80 + 14)
#define ID_DAP_VENDOR15			(0x80 + 15)
#define ID_DAP_VENDOR16			(0x80 + 16)
#define ID_DAP_VENDOR17			(0x80 + 17)
#define ID_DAP_VENDOR_NRF53_BOOTLOADER	ID_DAP_VENDOR14
#define ID_DAP_VENDOR_NRF91_BOOTLOADER	ID_DAP_VENDOR15
#define ID_DAP_VENDOR_NRF53_RESET	ID_DAP_VENDOR16
#define ID_DAP_VENDOR_NRF91_RESET	ID_DAP_VENDOR17

#define DAP_COMMAND_SUCCESS		0x00
#define DAP_COMMAND_FAILED		0xFF

#define NRF91_RESET_DURATION K_MSEC(100)
#define NRF91_BUTTON1_DURATION K_MSEC(1000)

static const struct gpio_dt_spec reset_pin =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), reset_gpios, {});
static const struct gpio_dt_spec button1_pin =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), button1_gpios, {});

static void nrf91_bootloader_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(nrf91_bootloader_work, nrf91_bootloader_work_handler);

static void nrf91_reset_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(nrf91_reset_work, nrf91_reset_work_handler);

static void nrf53_reset_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(nrf53_reset_work, nrf53_reset_work_handler);

/* Handler to make the nRF91 enter bootloader mode. */
static void nrf91_bootloader_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!(reset_pin.port && device_is_ready(reset_pin.port))) {
		LOG_ERR("reset pin not available");
		return;
	}

	if (!(button1_pin.port && device_is_ready(button1_pin.port))) {
		LOG_ERR("button pin not available");
		return;
	}

	/* assert both reset and button signals */
	gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT_LOW);
	gpio_pin_configure_dt(&button1_pin, GPIO_OUTPUT_LOW);
	/* wait for reset to be registered */
	k_sleep(NRF91_RESET_DURATION);
	gpio_pin_configure_dt(&reset_pin, GPIO_DISCONNECTED);
	/* hold down button for the bootloader to notice */
	k_sleep(NRF91_BUTTON1_DURATION);
	gpio_pin_configure_dt(&button1_pin, GPIO_DISCONNECTED);
}

/* Handler to reset the nRF91. */
static void nrf91_reset_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!(reset_pin.port && device_is_ready(reset_pin.port))) {
		LOG_ERR("reset pin not available");
		return;
	}

	/* assert both reset and button signals */
	gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT_LOW);
	/* wait for reset to be registered */
	k_sleep(NRF91_RESET_DURATION);
	gpio_pin_configure_dt(&reset_pin, GPIO_DISCONNECTED);
}

/* Handler to reset the nRF53. */
static void nrf53_reset_work_handler(struct k_work *work)
{
	sys_reboot(SYS_REBOOT_WARM);
}

/* This is a placeholder implementation until proper CMSIS-DAP support is available.
 * Only custom vendor commands are supported.
 */
size_t dap_execute_cmd(uint8_t *in, uint8_t *out)
{
	LOG_DBG("got command 0x%02X", in[0]);
	int ret;

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
	if (in[0] == ID_DAP_VENDOR_NRF53_BOOTLOADER) {
		ret = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
		if (ret) {
			LOG_ERR("Failed to set boot mode");
			goto error;
		}
		k_work_reschedule(&nrf53_reset_work, K_NO_WAIT);
		goto success;
	}
#endif /* CONFIG_RETENTION_BOOT_MODE */
	if (in[0] == ID_DAP_VENDOR_NRF91_BOOTLOADER) {
		if (!k_work_busy_get(&nrf91_bootloader_work.work)) {
			k_work_reschedule(&nrf91_bootloader_work, K_NO_WAIT);
		}
		goto success;
	}
	if (in[0] == ID_DAP_VENDOR_NRF53_RESET) {
		k_work_reschedule(&nrf53_reset_work, K_NO_WAIT);
		goto success;
	}
	if (in[0] == ID_DAP_VENDOR_NRF91_RESET) {
		if (!k_work_busy_get(&nrf91_reset_work.work)) {
			k_work_reschedule(&nrf91_reset_work, K_NO_WAIT);
		}
		goto success;
	}

error:
	/* default reply: command failed */
	out[0] = in[0];
	out[1] = DAP_COMMAND_FAILED;
	return 2;
success:
	out[0] = in[0];
	out[1] = DAP_COMMAND_SUCCESS;
	return 2;
}

#if defined(CONFIG_DK_LIBRARY)
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		if (!k_work_busy_get(&nrf91_reset_work.work)) {
			k_work_reschedule(&nrf91_reset_work, K_NO_WAIT);
		}
	}
}

static int button_handling_init(void)
{
	int err = dk_buttons_init(button_handler);

	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
	return 0;
}

SYS_INIT(button_handling_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* defined(CONFIG_DK_LIBRARY) */
