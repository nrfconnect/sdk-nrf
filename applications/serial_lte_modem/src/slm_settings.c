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
#include "lwm2m_carrier/slm_at_carrier.h"

LOG_MODULE_REGISTER(slm_config, CONFIG_SLM_LOG_LEVEL);

/**
 * Serial LTE Modem setting page for persistent data
 */

#if defined(CONFIG_SLM_CARRIER)
#include "slm_at_carrier.h"
int32_t slm_carrier_auto_connect = 1;
#endif

uint8_t slm_fota_type;
enum fota_stage slm_fota_stage = FOTA_STAGE_INIT;
enum fota_status slm_fota_status;
int32_t slm_fota_info;

bool slm_uart_configured;
struct uart_config slm_uart;

static int settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(name, "fota_type")) {
		if (len != sizeof(slm_fota_type))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_fota_type, len) > 0)
			return 0;
	} else if (!strcmp(name, "fota_stage")) {
		if (len != sizeof(slm_fota_stage))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_fota_stage, len) > 0)
			return 0;
	} else if (!strcmp(name, "fota_status")) {
		if (len != sizeof(slm_fota_status))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_fota_status, len) > 0)
			return 0;
	} else if (!strcmp(name, "fota_info")) {
		if (len != sizeof(slm_fota_info))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_fota_info, len) > 0)
			return 0;
#if defined(CONFIG_SLM_CARRIER)
	} else if (!strcmp(name, "auto_connect")) {
		if (len != sizeof(slm_carrier_auto_connect))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_carrier_auto_connect, len) > 0)
			return 0;
#endif
	} else if (!strcmp(name, "uart_configured")) {
		if (len != sizeof(slm_uart_configured))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_uart_configured, len) > 0)
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
	ret = settings_save_one("slm/fota_type", &(slm_fota_type), sizeof(slm_fota_type));
	if (ret) {
		LOG_ERR("save slm/fota_type failed: %d", ret);
		return ret;
	}
	ret = settings_save_one("slm/fota_stage", &(slm_fota_stage), sizeof(slm_fota_stage));
	if (ret) {
		LOG_ERR("save slm/fota_stage failed: %d", ret);
		return ret;
	}
	ret = settings_save_one("slm/fota_status", &(slm_fota_status), sizeof(slm_fota_status));
	if (ret) {
		LOG_ERR("save slm/fota_status failed: %d", ret);
		return ret;
	}
	ret = settings_save_one("slm/fota_info", &(slm_fota_info), sizeof(slm_fota_info));
	if (ret) {
		LOG_ERR("save slm/fota_info failed: %d", ret);
		return ret;
	}

	return 0;
}

void slm_settings_fota_init(void)
{
	slm_fota_type = DFU_TARGET_IMAGE_TYPE_ANY;
	slm_fota_stage = FOTA_STAGE_INIT;
	slm_fota_status = FOTA_STATUS_OK;
	slm_fota_info = 0;
}

#if defined(CONFIG_SLM_CARRIER)
int slm_settings_auto_connect_save(void)
{
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("slm/auto_connect",
		&(slm_carrier_auto_connect), sizeof(slm_carrier_auto_connect));
	if (ret) {
		LOG_ERR("save slm/auto_connect failed: %d", ret);
		return ret;
	}

	return 0;
}
#endif

int slm_settings_uart_save(void)
{
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("slm/uart_configured",
		&(slm_uart_configured), sizeof(slm_uart_configured));
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
