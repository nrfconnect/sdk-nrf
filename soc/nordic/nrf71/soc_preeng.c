/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Pre-ENG specific SoC initialization functions
 *
 * This module provides routines to initialize the SoC for the Pre-ENG variant
 * of the NRF7120.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(soc_preeng, CONFIG_SOC_LOG_LEVEL);

#define NRF_WIFICORE_RPURFBUS_BASE		     0x48020000UL
#define NRF_WIFICORE_RPURFBUS_RFCTRL		     ((NRF_WIFICORE_RPURFBUS_BASE) + 0x00010000UL)
#define NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS ((NRF_WIFICORE_RPURFBUS_RFCTRL) + 0x00000000UL)

#define RDW(addr)	(*(volatile unsigned int *)(addr))
#define WRW(addr, data) (*(volatile unsigned int *)(addr) = (data))

/* RF Playout Capture Register Configuration Structure */
struct rf_pocap_reg_config {
	/* Register offset from base */
	uint32_t offset;
	/* Register value to write */
	uint32_t value;
	/* Register name for logging */
	const char *reg_name;
	/* Description of what this register does */
	const char *desc;
};

int configure_playout_capture(uint32_t rx_mode, uint32_t tx_mode, uint32_t rx_holdoff_length,
			      uint32_t rx_wrap_length, uint32_t back_to_back_mode)
{
	unsigned int value;
	int i;

	LOG_INF("%s: Setting RF Playout Capture Config", __func__);

	/* Set AXI Master to APP so we can access the RF */
	WRW(NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS, 0x31);
	while (RDW(NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS) != 0x31) {
		/* busy-wait */
	}

	/* Data-driven register configuration */
	struct rf_pocap_reg_config reg_configs[] = {
		{.offset = 0x0000,
		 .value = (tx_mode << 1) | rx_mode,
		 .reg_name = "WLAN_RF_POCAP_CONFIG",
		 .desc = "RX/TX mode configuration"},
		{.offset = 0x0004,
		 .value = rx_holdoff_length & 0xFF,
		 .reg_name = "WLAN_RF_POCAP_RX_HOLDOFF_CONFIG",
		 .desc = "RX holdoff length configuration"},
		{.offset = 0x0008,
		 .value = rx_wrap_length,
		 .reg_name = "WLAN_RF_POCAP_RX_WRAP_CONFIG",
		 .desc = "RX wrap length configuration"},
		{.offset = 0x000C,
		 .value = (back_to_back_mode << 1), /* WLAN_POCAP_BACK_TO_BACK_MODE [1..1] */
		 .reg_name = "WLAN_POCAP_VERSION_CONFIG",
		 .desc = "Version and back-to-back mode configuration"}};

	/* Process register configurations - skip other configs if back-to-back mode is active */
	for (i = 0; i < ARRAY_SIZE(reg_configs); i++) {
		struct rf_pocap_reg_config *cfg = &reg_configs[i];

		/* If back-to-back mode is active, only configure the version register */
		if (back_to_back_mode && cfg->offset != 0x000C) {
			continue;
		}

		WRW(NRF_WIFICORE_RPURFBUS_BASE + cfg->offset, cfg->value);
		value = RDW(NRF_WIFICORE_RPURFBUS_BASE + cfg->offset);
		LOG_INF("%s: %s (0x%04x) = 0x%x - %s", __func__, cfg->reg_name, cfg->offset, value,
			cfg->desc);
	}

	LOG_INF("%s: RF Playout Capture Config completed", __func__);

	return 0;
}
