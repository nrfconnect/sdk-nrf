/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Zigbee Network Co-processor sample
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>
#include <zb_nrf_platform.h>
#include <zb_osif_ext.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <dk_buttons_and_leds.h>
#include <ncp/ncp_dev_api.h>


#if CONFIG_BOOTLOADER_MCUBOOT
#include <zephyr/dfu/mcuboot.h>
#endif

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define VENDOR_SPECIFIC_LED DK_LED2

#define VENDOR_SPECIFIC_LED_ACTION_OFF (0U)
#define VENDOR_SPECIFIC_LED_ACTION_ON (1U)
#define VENDOR_SPECIFIC_LED_ACTION_TOGGLE (2U)

#define VENDOR_SPECIFIC_REQUEST_LEN (1U)
#define VENDOR_SPECIFIC_RESPONSE_LEN (1U)

#define VENDOR_SPECIFIC_IND_LEN (1U)
#define VENDOR_SPECIFIC_IND_DELAY (ZB_TIME_ONE_SECOND * 3)


/* The state of a led controlled by ncp custom commands */
static zb_uint8_t vendor_specific_led_state = VENDOR_SPECIFIC_LED_ACTION_OFF;


zb_ret_t zb_osif_bootloader_run_after_reboot(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(rst0))
	int err = 0;
	const struct gpio_dt_spec rst0 = GPIO_DT_SPEC_GET(DT_ALIAS(rst0), gpios);

	if (!device_is_ready(rst0.port)) {
		return RET_ERROR;
	}

	err = gpio_pin_configure_dt(&rst0, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return RET_ERROR;
	}
#endif
	return RET_OK;
}

void zb_osif_bootloader_report_successful_loading(void)
{
#if CONFIG_BOOTLOADER_MCUBOOT
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret) {
			LOG_ERR("Couldn't confirm image: %d", ret);
		} else {
			LOG_INF("Marked image as OK");
		}
	}
#endif
}

static void custom_indication(zb_uint8_t buf, zb_uint16_t led_idx)
{
	zb_uint8_t *ind_data = zb_buf_initial_alloc(buf, VENDOR_SPECIFIC_IND_LEN);

	*ind_data = (zb_uint8_t)led_idx;

	zb_ncp_custom_indication(buf);
}

static void perform_custom_indication(zb_uint8_t led_idx)
{
	zb_buf_get_out_delayed_ext(custom_indication, led_idx, 0);
}

#if (defined ZBOSS_PLATFORM_MAJOR) && (ZBOSS_PLATFORM_MAJOR < 5U)
static zb_ret_t ncp_vendor_specific_req_handler(zb_uint8_t buf)
#else /* (defined ZBOSS_PLATFORM_MAJOR) && (ZBOSS_PLATFORM_MAJOR < 5U) */
static zb_uint16_t ncp_vendor_specific_req_handler(zb_uint8_t buf)
#endif /* (defined ZBOSS_PLATFORM_MAJOR) && (ZBOSS_PLATFORM_MAJOR < 5U) */
{
	/* request tsn */
	zb_uint8_t tsn = *ZB_BUF_GET_PARAM(buf, zb_uint8_t);
	/* actual payload passed by the request */
	zb_uint8_t *led_action = (zb_uint8_t *)zb_buf_begin(buf);

	zb_uint8_t resp_buf = zb_buf_get(ZB_FALSE, VENDOR_SPECIFIC_RESPONSE_LEN);
	zb_uint8_t *resp_data;
	ncp_hl_custom_resp_t *resp_args;

	if (resp_buf == ZB_BUF_INVALID) {
		LOG_ERR("Couldn't get buf");
		return RET_NO_MEMORY;
	}

	resp_data = zb_buf_initial_alloc(resp_buf, VENDOR_SPECIFIC_RESPONSE_LEN);
	resp_args = ZB_BUF_GET_PARAM(resp_buf, ncp_hl_custom_resp_t);

	if (zb_buf_len(buf) == VENDOR_SPECIFIC_REQUEST_LEN) {
		switch (*led_action) {
		case VENDOR_SPECIFIC_LED_ACTION_OFF:
			zb_osif_led_off(VENDOR_SPECIFIC_LED);
			resp_args->status = RET_OK;

			vendor_specific_led_state = 0;
			break;
		case VENDOR_SPECIFIC_LED_ACTION_ON:
			zb_osif_led_on(VENDOR_SPECIFIC_LED);
			resp_args->status = RET_OK;

			vendor_specific_led_state = 1;
			break;
		case VENDOR_SPECIFIC_LED_ACTION_TOGGLE:
			vendor_specific_led_state ^= 0x01;
			if (vendor_specific_led_state) {
				zb_osif_led_on(VENDOR_SPECIFIC_LED);
			} else {
				zb_osif_led_off(VENDOR_SPECIFIC_LED);
			}
			resp_args->status = RET_OK;

			break;
		default:
			resp_args->status = RET_ERROR;
			break;
		}
	} else {
		resp_args->status = RET_ERROR;
	}

	resp_args->tsn = tsn;
	*resp_data = vendor_specific_led_state;
	ZVUNUSED(zb_ncp_custom_response(resp_buf));

	return NCP_RET_LATER;
}

static void ncp_vendor_specific_init(void)
{
	zb_osif_led_button_init();

	zb_ncp_custom_register_request_cb(ncp_vendor_specific_req_handler);

	ZB_SCHEDULE_APP_ALARM(perform_custom_indication, (zb_uint8_t)VENDOR_SPECIFIC_LED,
						  VENDOR_SPECIFIC_IND_DELAY);
}

int main(void)
{
	LOG_INF("Starting Zigbee Network Co-processor sample");

#ifdef CONFIG_USB_DEVICE_STACK
	/* Enable USB device. */
	int ret = usb_enable(NULL);

	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("USB initialization failed");
		return ret;
	}

	/* Configure line control if flow control supported by Zigbee Async serial. */
	if (IS_ENABLED(CONFIG_ZIGBEE_UART_SUPPORTS_FLOW_CONTROL)) {
		const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_zigbee_uart));
		uint32_t dtr = 0U;

		if (!device_is_ready(uart_dev)) {
			LOG_ERR("UART device not ready");
			return -ENODEV;
		}

		while (true) {
			/* Break loop if line control can't be retrieved. */
			if (uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr)) {
				LOG_ERR("Couldn't get DTR signal during NCP serial initialization");
				break;
			}
			if (dtr) {
				break;
			}
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}

		/* Data Carrier Detect Modem - mark connection as established. */
		(void)uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DCD, 1);
		/* Data Set Ready - the NCP SoC is ready to communicate. */
		(void)uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DSR, 1);
	}

	/* Wait 1 sec for the host to do all settings */
	k_sleep(K_SECONDS(1));
#endif /* CONFIG_USB_DEVICE_STACK */

	zb_osif_ncp_set_nvram_filter();

	/* Setup ncp custom command handling */
	ncp_vendor_specific_init();

	/* Start Zigbee default thread */
	zigbee_enable();

	LOG_INF("Zigbee Network Co-processor sample started");

	return 0;
}
