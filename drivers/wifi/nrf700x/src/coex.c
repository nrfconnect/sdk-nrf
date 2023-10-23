/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include "coex.h"
#include "coex_struct.h"
#include "fmac_main.h"
#include "fmac_api.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF700X_LOG_LEVEL);

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
static struct nrf_wifi_ctx_zep *rpu_ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

#define CH_BASE_ADDRESS ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL
#define COEX_CONFIG_FIXED_PARAMS 4
#define COEX_REG_CFG_OFFSET_SHIFT 24

/* copied from uccp530_77_registers_ext_sys_bus.h of UCCP toolkit */
#define EXT_SYS_WLANSYSCOEX_CH_CONTROL 0x0000
#define ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL 0xA401BC00UL
#define EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE 0x0004
#define EXT_SYS_WLANSYSCOEX_CH_SR_INFO_STATUS 0x0040
#define EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_0 0x008C
#define EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_44 0x013C
#define EXT_SYS_WLANSYSCOEX_RESET_SHIFT 0

/* copied from uccp530_77_registers.h of UCCP toolkit */
#define ABS_PMB_WLAN_MAC_CTRL_PULSED_SOFTWARE_RESET 0xA5009A00UL

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	#define NRF_RADIO_COEX_NODE DT_NODELABEL(nrf_radio_coex)
	static const struct gpio_dt_spec btrf_switch_spec =
	GPIO_DT_SPEC_GET(NRF_RADIO_COEX_NODE, btrf_switch_gpios);
#endif

/* PTA registers configuration of Coexistence Hardware */
/* Separate antenna configuration, WLAN in 2.4GHz*/
const uint16_t config_buffer_SEA[] = {
	0x0019, 0x00F6, 0x0008, 0x0062, 0x00F5,
	0x00F5, 0x0019, 0x0019, 0x0074, 0x0074,
	0x0008, 0x01E2, 0x00D5, 0x00D5, 0x01F6,
	0x01F6, 0x0061, 0x0061, 0x01E2, 0x0008,
	0x0004, 0x0004, 0x0019, 0x0019, 0x0008,
	0x0008, 0x00F5, 0x00F5, 0x00D5, 0x00D5,
	0x0008, 0x01E2, 0x0051, 0x0051, 0x0074,
	0x0074, 0x00F6, 0x0019, 0x0062, 0x0019,
	0x00F6, 0x0008, 0x0062, 0x0008, 0x001A
};

/* Shared antenna configuration, WLAN in 2.4GHz */
const uint16_t config_buffer_SHA[] = {
	0x0019, 0x00F6, 0x0008, 0x00E2, 0x0015,
	0x00F5, 0x0019, 0x0019, 0x0004, 0x01F6,
	0x0008, 0x01E2, 0x00F5, 0x00F5, 0x01F6,
	0x01F6, 0x00E1, 0x00E1, 0x01E2, 0x0008,
	0x0004, 0x0004, 0x0019, 0x0019, 0x0008,
	0x0008, 0x0015, 0x00F5, 0x00F5, 0x00F5,
	0x0008, 0x01E2, 0x00E1, 0x00E1, 0x0004,
	0x01F6, 0x00F6, 0x0019, 0x00E2, 0x0019,
	0x00F6, 0x0008, 0x00E2, 0x0008, 0x001A
};

/* Shared/separate antennas, WLAN in 5GHz */
const uint16_t config_buffer_5G[] = {
	0x0039, 0x0076, 0x0028, 0x0062, 0x0075,
	0x0075, 0x0061, 0x0061, 0x0074, 0x0074,
	0x0060, 0x0060, 0x0075, 0x0075, 0x0064,
	0x0064, 0x0071, 0x0071, 0x0060, 0x0060,
	0x0064, 0x0064, 0x0061, 0x0061, 0x0060,
	0x0060, 0x0075, 0x0075, 0x0075, 0x0075,
	0x0060, 0x0060, 0x0071, 0x0071, 0x0074,
	0x0074, 0x0076, 0x0039, 0x0062, 0x0039,
	0x0076, 0x0028, 0x0062, 0x0028, 0x003A
};

/* non-PTA register configuration of coexistence hardware */
/* Shared antenna */
const uint32_t ch_config_sha[] = {
	0x00000028, 0x00000000, 0x001e1023, 0x00000000, 0x00000000,
	0x00000000, 0x00000021, 0x000002ca, 0x00000050, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000
};

/* Separate antennas */
const uint32_t ch_config_sep[] = {
	0x00000028, 0x00000000, 0x001e1023, 0x00000000, 0x00000000,
	0x00000000, 0x00000021, 0x000002ca, 0x00000055, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000
};

int nrf_wifi_coex_config_non_pta(bool separate_antennas)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	const uint32_t *config_buffer_ptr = NULL;
	uint32_t start_offset = 0;
	uint32_t num_reg_to_config = 1;
	uint32_t cmd_len, index;

	/* Offset from the base address of CH */
	start_offset = ((EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE -
	EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
	/* Number of registers to be configured */
	num_reg_to_config = ((EXT_SYS_WLANSYSCOEX_CH_SR_INFO_STATUS -
	EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE) >> 2) + 1;

	if (separate_antennas) {
		config_buffer_ptr = ch_config_sep;
	} else {
		config_buffer_ptr = ch_config_sha;
	}

	params.message_id = HW_CONFIGURATION;
	params.num_reg_to_config = num_reg_to_config;
	params.hw_to_config = COEX_HARDWARE;
	params.hw_block_base_addr = CH_BASE_ADDRESS;

	for (index = 0; index < num_reg_to_config; index++) {
		params.configbuf[index] = (start_offset << COEX_REG_CFG_OFFSET_SHIFT) |
			(*(config_buffer_ptr + index));
		start_offset++;
	}

	cmd_len = (COEX_CONFIG_FIXED_PARAMS + num_reg_to_config) * sizeof(uint32_t);

	status = nrf_wifi_fmac_conf_btcoex(rpu_ctx->rpu_ctx,
					   (void *)(&params), cmd_len);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		return -1;
	}

	return 0;
}

int nrf_wifi_coex_config_pta(enum nrf_wifi_pta_wlan_op_band wlan_band, bool separate_antennas)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	const uint16_t *config_buffer_ptr = NULL;
	uint32_t start_offset = 0;
	uint32_t num_reg_to_config = 1;
	uint32_t cmd_len, index;

	/* common for both SHA and SEA */
	/* Indicates offset from the base address of CH */
	start_offset = ((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_0 -
	EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
	/* Number of contiguous registers to be configured starting from base+offset */
	num_reg_to_config = ((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_44 -
	EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_0) >> 2) + 1;

	if (wlan_band == NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ) {
		/* WLAN operating in 2.4GHz */
		if (separate_antennas) {
			/* separate antennas configuration */
			config_buffer_ptr = config_buffer_SEA;
		} else {
			/* Shared antenna configuration */
			config_buffer_ptr = config_buffer_SHA;
		}
	} else if (wlan_band == NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ) {
		/* WLAN operating in 5GHz */
		config_buffer_ptr = config_buffer_5G;
	} else {
		return -EINVAL;
	}

	params.message_id = HW_CONFIGURATION;
	params.num_reg_to_config = num_reg_to_config;
	params.hw_to_config = COEX_HARDWARE;
	params.hw_block_base_addr = CH_BASE_ADDRESS;

	for (index = 0; index < num_reg_to_config; index++) {
		params.configbuf[index] = (start_offset << COEX_REG_CFG_OFFSET_SHIFT) |
			(*(config_buffer_ptr+index));
		start_offset++;
	}

	cmd_len = (COEX_CONFIG_FIXED_PARAMS + num_reg_to_config) * sizeof(uint32_t);

	status = nrf_wifi_fmac_conf_btcoex(rpu_ctx->rpu_ctx,
					   (void *)(&params), cmd_len);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		return -1;
	}

	return 0;
}

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
int nrf_wifi_config_sr_switch(bool separate_antennas)
{
	int ret;

	if (!device_is_ready(btrf_switch_spec.port)) {
		LOG_ERR("Unable to open GPIO device\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&btrf_switch_spec, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Unable to configure GPIO device\n");
		return -1;
	}

	if (separate_antennas) {
		gpio_pin_set_dt(&btrf_switch_spec, 0x0);
		LOG_INF("GPIO P1.10 set to 0\n");
	} else {
		gpio_pin_set_dt(&btrf_switch_spec, 0x1);
		LOG_INF("GPIO P1.10 set to 1\n");
	}

	LOG_DBG("Successfully configured GPIO P1.10\n");

	return 0;
}
#endif

int nrf_wifi_coex_hw_reset(void)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	uint32_t num_reg_to_config = 1;
	uint32_t start_offset = 0;
	uint32_t index = 0;
	uint32_t coex_hw_reset = 1;
	uint32_t cmd_len;

	/* reset CH */
	params.message_id = HW_CONFIGURATION;
	params.num_reg_to_config = num_reg_to_config;
	params.hw_to_config = COEX_HARDWARE;
	params.hw_block_base_addr = CH_BASE_ADDRESS;

	start_offset = ((EXT_SYS_WLANSYSCOEX_CH_CONTROL - EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
	params.configbuf[index] = (start_offset << COEX_REG_CFG_OFFSET_SHIFT) |
	(coex_hw_reset << EXT_SYS_WLANSYSCOEX_RESET_SHIFT);

	cmd_len = (COEX_CONFIG_FIXED_PARAMS + num_reg_to_config) * sizeof(uint32_t);

	status = nrf_wifi_fmac_conf_btcoex(rpu_ctx->rpu_ctx,
				(void *)(&params), cmd_len);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("CH reset configuration failed\n");
		return -1;
	}

	return 0;
}
