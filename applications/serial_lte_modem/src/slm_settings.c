/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/uart.h>
#include <dfu/dfu_target.h>
#include "slm_at_fota.h"
#include "slm_settings.h"

LOG_MODULE_REGISTER(slm_config, CONFIG_SLM_LOG_LEVEL);

/**
 * Serial LTE Modem setting page for persistent data
 */
uint8_t fota_type;		/* FOTA: image type */
enum fota_stage fota_stage = FOTA_STAGE_INIT;		/* FOTA: stage of FOTA process */
enum fota_status fota_status;		/* FOTA: OK/Error status */
int32_t fota_info;		/* FOTA: failure cause in case of error or download percentage*/

bool uart_configured;		/* UART: first-time configured */
struct uart_config slm_uart;	/* UART: config */

static int settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(name, "fota_type")) {
		if (len != sizeof(fota_type))
			return -EINVAL;
		if (read_cb(cb_arg, &fota_type, len) > 0)
			return 0;
	} else if (!strcmp(name, "fota_stage")) {
		if (len != sizeof(fota_stage))
			return -EINVAL;
		if (read_cb(cb_arg, &fota_stage, len) > 0)
			return 0;
	} else if (!strcmp(name, "fota_status")) {
		if (len != sizeof(fota_status))
			return -EINVAL;
		if (read_cb(cb_arg, &fota_status, len) > 0)
			return 0;
	} else if (!strcmp(name, "fota_info")) {
		if (len != sizeof(fota_info))
			return -EINVAL;
		if (read_cb(cb_arg, &fota_info, len) > 0)
			return 0;
	} else if (!strcmp(name, "uart_configured")) {
		if (len != sizeof(uart_configured))
			return -EINVAL;
		if (read_cb(cb_arg, &uart_configured, len) > 0)
			return 0;
	} else if (!strcmp(name, "uart_baudrate")) {
		if (len != sizeof(slm_uart.baudrate))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_uart.baudrate, len) > 0)
			return 0;
	} else if (!strcmp(name, "uart_parity")) {
		if (len != sizeof(slm_uart.parity))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_uart.parity, len) > 0)
			return 0;
	} else if (!strcmp(name, "uart_stop_bits")) {
		if (len != sizeof(slm_uart.stop_bits))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_uart.stop_bits, len) > 0)
			return 0;
	} else if (!strcmp(name, "uart_data_bits")) {
		if (len != sizeof(slm_uart.data_bits))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_uart.data_bits, len) > 0)
			return 0;
	} else if (!strcmp(name, "uart_flow_ctrl")) {
		if (len != sizeof(slm_uart.flow_ctrl))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_uart.flow_ctrl, len) > 0)
			return 0;
	}

	return -ENOENT;
}

static struct settings_handler slm_settings_conf = {
	.name = "slm",
	.h_set = settings_set
};

int slm_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Init setting failed: %d", ret);
		return ret;
	}
	ret = settings_register(&slm_settings_conf);
	if (ret) {
		LOG_ERR("Register setting failed: %d", ret);
		return ret;
	}
	ret = settings_load_subtree("slm");
	if (ret) {
		LOG_ERR("Load setting failed: %d", ret);
	}

	return ret;
}

int slm_settings_fota_save(void)
{
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("slm/fota_type", &(fota_type), sizeof(fota_type));
	if (ret) {
		LOG_ERR("save slm/fota_type failed: %d", ret);
		return ret;
	}
	ret = settings_save_one("slm/fota_stage", &(fota_stage), sizeof(fota_stage));
	if (ret) {
		LOG_ERR("save slm/fota_stage failed: %d", ret);
		return ret;
	}
	ret = settings_save_one("slm/fota_status", &(fota_status), sizeof(fota_status));
	if (ret) {
		LOG_ERR("save slm/fota_status failed: %d", ret);
		return ret;
	}
	ret = settings_save_one("slm/fota_info", &(fota_info), sizeof(fota_info));
	if (ret) {
		LOG_ERR("save slm/fota_info failed: %d", ret);
		return ret;
	}

	return 0;
}

void slm_settings_fota_init(void)
{
	fota_type = DFU_TARGET_IMAGE_TYPE_ANY;
	fota_stage = FOTA_STAGE_INIT;
	fota_status = FOTA_STATUS_OK;
	fota_info = 0;
}

int slm_settings_uart_save(void)
{
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("slm/uart_configured",
		&(uart_configured), sizeof(uart_configured));
	if (ret) {
		return ret;
	}
	ret = settings_save_one("slm/uart_baudrate",
		&(slm_uart.baudrate), sizeof(slm_uart.baudrate));
	if (ret) {
		return ret;
	}
	ret = settings_save_one("slm/uart_parity",
		&(slm_uart.parity), sizeof(slm_uart.parity));
	if (ret) {
		return ret;
	}
	ret = settings_save_one("slm/uart_stop_bits",
		&(slm_uart.stop_bits), sizeof(slm_uart.stop_bits));
	if (ret) {
		return ret;
	}
	ret = settings_save_one("slm/uart_data_bits",
		&(slm_uart.data_bits), sizeof(slm_uart.data_bits));
	if (ret) {
		return ret;
	}
	ret = settings_save_one("slm/uart_flow_ctrl",
		&(slm_uart.flow_ctrl), sizeof(slm_uart.flow_ctrl));
	if (ret) {
		return ret;
	}

	return 0;
}
