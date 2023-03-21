/*
 * Copyright (c) 2023 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lcz_fem_power, CONFIG_LCZ_FEM_LOG_LEVEL);

#include <mpsl.h>
#include <mpsl_tx_power.h>

#include "fem_power_table.h"

#if defined(CONFIG_MPSL_FEM_NRF21540_GPIO_SPI_SUPPORT) || defined(CONFIG_MPSL_FEM_POWER_MODEL) ||  \
	defined(CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL)
#error "Unsupported MPSL FEM configuration"
#endif

/* Mode pin is grounded on BL5340PA */
#if CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB != 20
#error "TX Gain must be set to 20"
#endif

#if defined(CONFIG_LCZ_FEM_LOG_POWER_ADJUST_TABLE)
static void log_power_adjust_table(void)
{
	const int8_t list[] = { 30, 20, 12,  11,  10,  9,   8,	 7,   6,   5,  4,
				3,  2,	1,   0,	  -1,  -2,  -3,	 -4,  -5,  -6, -7,
				-8, -9, -10, -11, -12, -16, -20, -30, -40, -50 };
	int i;

	for (i = 0; i < sizeof(list); i++) {
		LOG_INF("%3d -> %3d", list[i], mpsl_tx_power_radio_supported_power_adjust(list[i]));
	}
}
#endif

static int lcz_fem_power_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int r;

	do {
		if (!mpsl_is_initialized()) {
			LOG_ERR("%s: MPSL is NOT initialized", __func__);
			r = -EPERM;
			break;
		}

		LOG_INF("%s", POWER_TABLE_STR);

#if defined(CONFIG_BT_CTLR_PHY_CODED)
		/* Use power tables based on antenna type and region */
		r = mpsl_tx_power_channel_map_set(&ENV_125K);
		if (r < 0) {
			LOG_ERR("Unable to set 125K power envelope");
			break;
		}

		r = mpsl_tx_power_channel_map_set(&ENV_500K);
		if (r < 0) {
			LOG_ERR("Unable to set 500K power envelope");
			break;
		}
#endif

#if defined(CONFIG_BT)
		r = mpsl_tx_power_channel_map_set(&ENV_1M);
		if (r < 0) {
			LOG_ERR("Unable to set 1M power envelope");
			break;
		}

		r = mpsl_tx_power_channel_map_set(&ENV_2M);
		if (r < 0) {
			LOG_ERR("Unable to set 2M power envelope");
			break;
		}
#endif

#if defined(CONFIG_NRF_802154_SL)
		r = mpsl_tx_power_channel_map_set(&ENV_802154);
		if (r < 0) {
			LOG_ERR("Unable to set 802.15.4 power envelope");
			break;
		}
#endif

	} while (0);

	LOG_INF("%s status: %d", __func__, r);

#if defined(CONFIG_LCZ_FEM_LOG_POWER_ADJUST_TABLE)
	log_power_adjust_table();
#endif

	return r;
}

SYS_INIT(lcz_fem_power_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
