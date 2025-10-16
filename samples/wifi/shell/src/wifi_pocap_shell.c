/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi shell sample main function
 */

 #include <zephyr/sys/printk.h>
 #include <zephyr/kernel.h>
 #include <zephyr/shell/shell.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pocap_mode, CONFIG_LOG_DEFAULT_LEVEL);

#define NRF_WIFICORE_RPURFBUS_BASE        0x48020000UL
#define NRF_WIFICORE_RPURFBUS_RFCTRL ((NRF_WIFICORE_RPURFBUS_BASE) + 0x00010000UL)
#define NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS ((NRF_WIFICORE_RPURFBUS_RFCTRL) + 0x00000000UL)

#define RDW(addr)           (*(volatile unsigned int *)(addr))
#define WRW(addr, data)     (*(volatile unsigned int *)(addr) = (data))


/* RF Playout Capture Register Configuration Structure */
struct rf_pocap_reg_config {
	uint32_t offset;		/* Register offset from base */
	uint32_t value;		/* Register value to write */
	const char *reg_name;	/* Register name for logging */
	const char *desc;		/* Description of what this register does */
};

void configurePlayoutCapture(uint32_t rx_mode, uint32_t tx_mode, uint32_t rx_holdoff_length, uint32_t rx_wrap_length, uint32_t back_to_back_mode)
{
#if (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP || CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU)
	unsigned int value;
	LOG_INF("%s: Setting RF Playout Capture Config", __func__);

	// Set AXI Master to APP so we can access the RF
	WRW(NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS, 0x31);
	while(RDW(NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS) != 0x31);

	/* Data-driven register configuration */
	struct rf_pocap_reg_config reg_configs[] = {
		{
			.offset = 0x0000,
			.value = (tx_mode << 1) | rx_mode,
			.reg_name = "WLAN_RF_POCAP_CONFIG",
			.desc = "RX/TX mode configuration"
		},
		{
			.offset = 0x0004,
			.value = rx_holdoff_length & 0xFF,
			.reg_name = "WLAN_RF_POCAP_RX_HOLDOFF_CONFIG",
			.desc = "RX holdoff length configuration"
		},
		{
			.offset = 0x0008,
			.value = rx_wrap_length,
			.reg_name = "WLAN_RF_POCAP_RX_WRAP_CONFIG",
			.desc = "RX wrap length configuration"
		},
		{
			.offset = 0x000C,
			.value = (back_to_back_mode << 1), /* WLAN_POCAP_BACK_TO_BACK_MODE [1..1] */
			.reg_name = "WLAN_POCAP_VERSION_CONFIG",
			.desc = "Version and back-to-back mode configuration"
		}
	};

	/* Process register configurations - skip other configs if back-to-back mode is active */
	for (int i = 0; i < sizeof(reg_configs) / sizeof(reg_configs[0]); i++) {
		struct rf_pocap_reg_config *cfg = &reg_configs[i];

		/* If back-to-back mode is active, only configure the version register */
		if (back_to_back_mode && cfg->offset != 0x000C) {
			continue;
		}

		WRW(NRF_WIFICORE_RPURFBUS_BASE + cfg->offset, cfg->value);
		value = RDW(NRF_WIFICORE_RPURFBUS_BASE + cfg->offset);
		LOG_INF("%s: %s (0x%04x) = 0x%x - %s",
			__func__, cfg->reg_name, cfg->offset, value, cfg->desc);
	}

	LOG_INF("%s: RF Playout Capture Config completed", __func__);
#endif /* (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP && CONFIG_EMULATOR_SYSTEMC) || (CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU) */
}

static int cmd_wifi_pocap(const struct shell *sh, size_t argc, char *argv[])
{
	/* Default values */
	uint32_t rx_mode = 0;
	uint32_t tx_mode = 0;
	uint32_t rx_holdoff_length = 0;
	uint32_t rx_wrap_length = 0;
	uint32_t back_to_back_mode = 0;
	int opt;

	optind = 1;

	while ((opt = getopt(argc, argv, "r:t:h:w:b:")) != -1) {
		switch (opt) {
		case 'r':
			rx_mode = (uint32_t)atoi(optarg);
			break;
		case 't':
			tx_mode = (uint32_t)atoi(optarg);
			break;
		case 'h':
			rx_holdoff_length = (uint32_t)atoi(optarg);
			break;
		case 'w':
			rx_wrap_length = (uint32_t)atoi(optarg);
			break;
		case 'b':
			back_to_back_mode = (uint32_t)atoi(optarg);
			break;
		default:
			shell_print(
				sh,
				"Usage: wifi_pocap [-b back_to_back_mode] [other options]\n"
				"For back_to_back_mode=0, you may use:\n"
				"    -r rx_mode -t tx_mode -h rx_holdoff_length -w rx_wrap_length\n"
				"Defaults: rx_mode=0 tx_mode=0 rx_holdoff_length=0 rx_wrap_length=0 back_to_back_mode=0"
			);
			return -EINVAL;
		}
	}

	if (back_to_back_mode) {
		/* Only back to back mode config; others are ignored */
		shell_print(sh,
			    "Configuring with: back_to_back_mode=%u (other parameters ignored in B2B mode)",
			    back_to_back_mode);
		configurePlayoutCapture(0, 0, 0, 0, back_to_back_mode);
	} else {
		shell_print(
			sh,
			"Configuring with: rx_mode=%u, tx_mode=%u, rx_holdoff_length=%u, rx_wrap_length=%u, back_to_back_mode=0",
			rx_mode, tx_mode, rx_holdoff_length, rx_wrap_length);
		configurePlayoutCapture(rx_mode, tx_mode, rx_holdoff_length, rx_wrap_length, 0);
	}
	return 0;
}

SHELL_CMD_REGISTER(wifi_pocap, NULL, "Configure RF Playout Capture", cmd_wifi_pocap);
