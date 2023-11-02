/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <bluetooth/services/ddfs.h>
#include <zephyr/random/random.h>
#include <dm.h>

#include "service.h"
#include "peer.h"


static int dm_ranging_mode_set(uint8_t mode)
{
	if (mode == BT_DDFS_DM_RANGING_MODE_RTT) {
		peer_ranging_mode_set(DM_RANGING_MODE_RTT);
	} else if (mode == BT_DDFS_DM_RANGING_MODE_MCPD) {
		peer_ranging_mode_set(DM_RANGING_MODE_MCPD);
	} else {
		return -1;
	}

	printk("DM set ranging mode to %s\n", mode == BT_DDFS_DM_RANGING_MODE_RTT ? "RTT" : "MCPD");
	return 0;
}

static int dm_config_read(struct bt_ddfs_dm_config *config)
{
	enum dm_ranging_mode ranging_mode;

	if (!config) {
		return -1;
	}

	ranging_mode = peer_ranging_mode_get();
	if (ranging_mode == DM_RANGING_MODE_RTT) {
		config->mode = BT_DDFS_DM_RANGING_MODE_RTT;
	} else if (ranging_mode == DM_RANGING_MODE_MCPD) {
		config->mode = BT_DDFS_DM_RANGING_MODE_MCPD;
	} else {
		return -1;
	}

	if (IS_ENABLED(CONFIG_DM_HIGH_PRECISION_CALC)) {
		config->high_precision = true;
	} else {
		config->high_precision = false;
	}

	return 0;
}

static uint16_t meter_to_decimeter(float value)
{
	float val;

	val = (value < 0 ? 0 : value) * 10;
	return val < UINT16_MAX ? (uint16_t)val : UINT16_MAX;
}

void service_distance_measurement_update(const bt_addr_le_t *addr, const struct dm_result *result)
{
	struct bt_ddfs_distance_measurement measurement;
	int err;

	if (!addr || !result) {
		return;
	}

	if (result->quality == DM_QUALITY_OK) {
		measurement.quality = BT_DDFS_QUALITY_OK;
	} else if (result->quality == DM_QUALITY_POOR) {
		measurement.quality = BT_DDFS_QUALITY_POOR;
	} else if (result->quality == DM_QUALITY_DO_NOT_USE) {
		measurement.quality = BT_DDFS_QUALITY_DO_NOT_USE;
	} else if (result->quality == DM_QUALITY_CRC_FAIL) {
		measurement.quality = BT_DDFS_QUALITY_DO_NOT_USE;
	} else {
		measurement.quality = BT_DDFS_QUALITY_NONE;
	}

	if (result->ranging_mode == DM_RANGING_MODE_RTT) {
		measurement.ranging_mode = BT_DDFS_DM_RANGING_MODE_RTT;
		measurement.dist_estimates.rtt.rtt =
				meter_to_decimeter(result->dist_estimates.rtt.rtt);
	} else {
		measurement.ranging_mode = BT_DDFS_DM_RANGING_MODE_MCPD;
		measurement.dist_estimates.mcpd.ifft =
				meter_to_decimeter(result->dist_estimates.mcpd.ifft);
		measurement.dist_estimates.mcpd.phase_slope =
				meter_to_decimeter(result->dist_estimates.mcpd.phase_slope);
		measurement.dist_estimates.mcpd.rssi_openspace =
				meter_to_decimeter(result->dist_estimates.mcpd.rssi_openspace);
		measurement.dist_estimates.mcpd.best =
				meter_to_decimeter(result->dist_estimates.mcpd.best);
#ifdef CONFIG_DM_HIGH_PRECISION_CALC
		measurement.dist_estimates.mcpd.high_precision =
				meter_to_decimeter(result->dist_estimates.mcpd.high_precision);
#endif
	}

	bt_addr_le_copy(&measurement.bt_addr, addr);

	err = bt_ddfs_distance_measurement_notify(NULL, &measurement);
	if (err && (err != -EACCES) && (err != -ENOTCONN)) {
		printk("Failed to send distance measurement (err %d)\n", err);
	}
}

void service_azimuth_elevation_simulation(void)
{
	int err;

	uint32_t rand = sys_rand32_get();
	struct bt_ddfs_elevation_measurement elevation;
	struct bt_ddfs_azimuth_measurement azimuth;
	const bt_addr_le_t bt_addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA},
	};

	elevation.quality = rand % (BT_DDFS_QUALITY_DO_NOT_USE + 1);
	elevation.value	= -90 + rand % 91;
	bt_addr_le_copy(&elevation.bt_addr, &bt_addr);

	azimuth.quality = rand % (BT_DDFS_QUALITY_DO_NOT_USE + 1);
	azimuth.value = rand % 360;
	bt_addr_le_copy(&azimuth.bt_addr, &bt_addr);

	err = bt_ddfs_elevation_measurement_notify(NULL, &elevation);
	if (err && (err != -EACCES) && (err != -ENOTCONN)) {
		printk("Failed to send elevation measurement (err %d)\n", err);
	}

	err = bt_ddfs_azimuth_measurement_notify(NULL, &azimuth);
	if (err && (err != -EACCES) && (err != -ENOTCONN)) {
		printk("Failed to send azimuth measurement (err %d)\n", err);
	}
}

static const struct bt_ddfs_cb cb = {
	.dm_ranging_mode_set = dm_ranging_mode_set,
	.dm_config_read = dm_config_read,
	.am_notification_config_changed = NULL,
	.dm_notification_config_changed = NULL,
	.em_notification_config_changed = NULL
};

int service_ddfs_init(void)
{
	struct bt_ddfs_init_params ddfs_init = {0};

	ddfs_init.dm_features.ranging_mode_rtt = 1;
	ddfs_init.dm_features.ranging_mode_mcpd = 1;
	ddfs_init.cb = &cb;

	return bt_ddfs_init(&ddfs_init);
}
