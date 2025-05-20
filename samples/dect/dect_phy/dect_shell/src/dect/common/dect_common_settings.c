/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>
#include <zephyr/random/random.h>
#include <nrf_modem_dect_phy.h>

#include "desh_print.h"
#include "dect_common.h"
#include "dect_common_settings.h"

static const struct dect_phy_settings_common_tx phy_tx_common_settings = {
	.power_dbm = DECT_PHY_SETT_DEFAULT_TX_POWER_DBM,
	.mcs = DECT_PHY_SETT_DEFAULT_TX_MCS,
};

static const struct dect_phy_settings_common_rx phy_rx_common_settings = {
	.expected_rssi_level = DECT_PHY_SETT_DEFAULT_RX_EXPECTED_RSSI_LEVEL,
};

static const struct dect_phy_settings_harq phy_harq_common_settings = {
	.mdm_init_harq_expiry_time_us = 2500000,
	.mdm_init_harq_process_count = 4,
	.harq_feedback_rx_delay_subslot_count = 4,
	.harq_feedback_rx_subslot_count = 18,
	.harq_feedback_tx_delay_subslot_count = 4,
};

static const struct dect_phy_settings_scheduler phy_scheduler_common_settings = {
	.scheduling_delay_us = DECT_PHY_API_SCHEDULER_OFFSET_US,
};

static const struct dect_phy_settings_rssi_scan phy_rssi_scan_settings_data = {
	.result_verdict_type = DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_ALL,
	.time_per_channel_ms =
		(DECT_PHY_SETT_DEFAULT_BEACON_TX_INTERVAL_SECS * 1000) + 10,
	.type_subslots_params.scan_suitable_percent = DECT_PHY_SETT_DEFAULT_SCAN_SUITABLE_PERCENT,

	.busy_threshold = DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MAX,
	.free_threshold = DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MIN,
};

static const struct dect_phy_settings_common phy_common_settings_data = {
	.network_id = DECT_PHY_DEFAULT_NETWORK_ID,
	.transmitter_id = DECT_PHY_DEFAULT_TRANSMITTER_LONG_RD_ID,
	.band_nbr = 1,
	.activate_at_startup = true,
	.startup_radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY,
};

static const struct dect_phy_settings phy_settings_data_defaults = {
	.common = phy_common_settings_data,
	.scheduler = phy_scheduler_common_settings,
	.rssi_scan = phy_rssi_scan_settings_data,
	.harq = phy_harq_common_settings,
	.tx = phy_tx_common_settings,
	.rx = phy_rx_common_settings,
};
static struct dect_phy_settings phy_settings_data = phy_settings_data_defaults;

/**************************************************************************************************/

int dect_common_settings_write(struct dect_phy_settings *dect_common_sett)
{
	int ret = settings_save_one(DECT_PHY_SETT_TREE_KEY "/" DECT_PHY_SETT_COMMON_CONFIG_KEY,
				    dect_common_sett, sizeof(struct dect_phy_settings));
	uint16_t orig_short_rd_id = phy_settings_data.common.short_rd_id;

	if (ret) {
		desh_error("Cannot save dect common settings, err: %d", ret);
		return ret;
	}

	phy_settings_data = *dect_common_sett;

	/* Short RD ID is generated at boot but we store it in settings */
	phy_settings_data.common.short_rd_id = orig_short_rd_id;
	desh_print("dect common settings saved");

	return ret;
}

int dect_common_settings_defaults_set(void)
{
	int ret = settings_save_one(DECT_PHY_SETT_TREE_KEY "/" DECT_PHY_SETT_COMMON_CONFIG_KEY,
				    &phy_settings_data_defaults, sizeof(struct dect_phy_settings));
	uint16_t orig_short_rd_id = phy_settings_data.common.short_rd_id;

	if (ret) {
		desh_error("Cannot set dect default settings, err: %d", ret);
		return ret;
	}
	memcpy(&phy_settings_data, &phy_settings_data_defaults, sizeof(struct dect_phy_settings));

	/* Short RD ID is generated at boot but we store it in settings */
	phy_settings_data.common.short_rd_id = orig_short_rd_id;

	desh_print("Default settings set.");

	return ret;
}

int dect_common_settings_read(struct dect_phy_settings *dect_common_sett)
{
	memcpy(dect_common_sett, &phy_settings_data, sizeof(struct dect_phy_settings));
	return 0;
}

struct dect_phy_settings *dect_common_settings_ref_get(void)
{
	return &phy_settings_data;
}

static int dect_common_settings_handler(const char *key, size_t len, settings_read_cb read_cb,
					void *cb_arg)
{
	int ret = 0;

	if (strcmp(key, DECT_PHY_SETT_COMMON_CONFIG_KEY) == 0) {
		ret = read_cb(cb_arg, &phy_settings_data, sizeof(phy_settings_data));
		if (ret < 0) {
			printk("Failed to read dect phy_settings_data, error: %d", ret);
			return ret;
		}
		return 0;
	}
	return -ENOENT;
}

static struct settings_handler cfg = {.name = DECT_PHY_SETT_TREE_KEY,
				      .h_set = dect_common_settings_handler};

int dect_common_settings_init(void)
{
	int ret = 0;

	/* Set the initial defaults from the code: */
	phy_settings_data.common = phy_common_settings_data;

	/* MAC spec: 4.2.3.3: short rd id is in range of 0x0001-0xFFFE */
	phy_settings_data.common.short_rd_id = (sys_rand16_get() % 0xFFFE) + 1;

	phy_settings_data.scheduler = phy_scheduler_common_settings;
	phy_settings_data.rssi_scan = phy_rssi_scan_settings_data;

	ret = settings_subsys_init();
	if (ret) {
		printk("(%s): failed to initialize settings subsystem, error: %d", (__func__), ret);
		return ret;
	}
	ret = settings_register(&cfg);
	if (ret) {
		printk("Cannot register settings handler %d", ret);
		return ret;
	}
	ret = settings_load_subtree(DECT_PHY_SETT_TREE_KEY);
	if (ret) {
		printk("(%s): cannot load settings %d", (__func__), ret);
		return ret;
	}
	return 0;
}

SYS_INIT(dect_common_settings_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
