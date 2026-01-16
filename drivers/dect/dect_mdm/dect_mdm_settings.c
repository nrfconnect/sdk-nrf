/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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

#include <nrf_modem_dect.h>

#include "dect_mdm_utils.h"
#include "dect_mdm_settings.h"

K_SEM_DEFINE(dect_settings_init_sema, 0, 1);

/* Default nrf91 settings */
static const struct dect_settings_association association_data = {
	.max_beacon_rx_failures =
		CONFIG_DECT_MDM_DEFAULT_PT_ASSOCIATION_MAX_BEACON_RX_FAILURES,
	.min_sensitivity_dbm = CONFIG_DECT_MDM_DEFAULT_PT_ASSOCIATION_MIN_SENSITIVITY_DBM,
};

static const struct dect_settings_network_beacon nw_beacon_data = {
	.beacon_period = DECT_NW_BEACON_PERIOD_2000MS,
	.channel = DECT_NW_BEACON_CHANNEL_NOT_USED,
};

static const struct dect_settings_cluster cluster_beacon_data = {
	.beacon_period = DECT_CLUSTER_BEACON_PERIOD_2000MS,
	.max_beacon_tx_power_dbm = 10,
	.max_cluster_power_dbm = 13,
	.max_num_neighbors = CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT,
	.channel_loaded_percent = CONFIG_DECT_MDM_DEFAULT_FT_CLUSTER_CHANNEL_LOADED_PERCENT,
	.neighbor_inactivity_disconnect_timer_ms =
		CONFIG_DECT_MDM_DEFAULT_FT_NEIGHBOR_INACTIVITY_DISCONNECT_TIMER_MS,
};

static const struct dect_settings_auto_start auto_start_data = {
	.activate = true, /* Activate DECT NR+ stack on bootup */
};

static const struct dect_settings_rssi_scan rssi_scan_data = {
	.time_per_channel_ms = 200,
	.scan_suitable_percent = CONFIG_DECT_MDM_DEFAULT_RSSI_SCAN_SUITABLE_PERCENT,
	.busy_threshold_dbm = DECT_MDM_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MAX,
	.free_threshold_dbm = DECT_MDM_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MIN,
};

static const struct dect_settings_security_conf security_configuration_data = {
	.mode = DECT_SECURITY_MODE_1, /* Enabled */
	.integrity_key = {0x4a, 0x75, 0x73, 0x74, 0x41, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74,
			  0x21, 0x21, 0x21, 0x21},
	.cipher_key = {0x4a, 0x75, 0x73, 0x74, 0x41, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x21,
		       0x21, 0x21, 0x21},
};

static const struct dect_settings common_settings_data = {
	.region = DECT_SETTINGS_REGION_EU,
	.identities.network_id = DECT_DEFAULT_NW_ID,
	.identities.transmitter_long_rd_id = DECT_MDM_LONG_RD_ID_ID_NOT_SET,
	.device_type = DECT_MDM_DEFAULT_DEVICE_TYPE,
	.auto_start = auto_start_data,
	.band_nbr = 1,
	.power_save = false,
	.tx.max_power_dbm = 19,
	.tx.max_mcs = 4,
	.rssi_scan = rssi_scan_data,
	.cluster = cluster_beacon_data,
	.network_join = {
		.target_ft_long_rd_id = DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY,
	},
	.nw_beacon = nw_beacon_data,
	.association = association_data,
	.sec_conf = security_configuration_data,
};

static const struct dect_mdm_settings settings_data_defaults = {
	.net_mgmt_common = common_settings_data,
};
static struct dect_mdm_settings settings_data = settings_data_defaults;

LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

BUILD_ASSERT(CONFIG_DECT_MDM_RANDOM_RANGE_END > CONFIG_DECT_MDM_RANDOM_RANGE_START);

static void dect_mdm_settings_tx_id_default_set(void)
{
	if (settings_data.net_mgmt_common.identities.transmitter_long_rd_id ==
		DECT_MDM_LONG_RD_ID_ID_NOT_SET) {
		uint32_t new_tx_id = DECT_MDM_LONG_RD_ID_ID_NOT_SET;

		if (IS_ENABLED(CONFIG_DECT_DEFAULT_LONG_RD_ID_TYPE_HARD_CODED)) {
			/* Use hard coded value */
			new_tx_id = CONFIG_DECT_DEFAULT_LONG_RD_ID;
		} else {
			/* MAC spec: 4.2.3.2: long rd id is in range of 0x00000001-0xFFFFFFFD */
			uint32_t random_value = sys_rand32_get();

			__ASSERT_NO_MSG(
				IS_ENABLED(CONFIG_DECT_DEFAULT_LONG_RD_ID_TYPE_RANDOM));
			random_value =
				(random_value % (CONFIG_DECT_MDM_RANDOM_RANGE_END -
						 CONFIG_DECT_MDM_RANDOM_RANGE_START + 1)) +
				CONFIG_DECT_MDM_RANDOM_RANGE_START;
			new_tx_id = random_value;
		}
		/* Write defaults */
		struct dect_mdm_settings *current_sett_ptr = dect_mdm_settings_ref_get();
		struct dect_settings *current_sett = &current_sett_ptr->net_mgmt_common;
		int ret;

		current_sett->identities.transmitter_long_rd_id = new_tx_id;

		ret = settings_save_one(
			DECT_MDM_SETT_TREE_KEY "/" DECT_MDM_SETT_COMMON_CONFIG_KEY,
			current_sett_ptr, sizeof(struct dect_mdm_settings));
		if (ret) {
			LOG_ERR("Cannot save tx id to settings, err: %d", ret);
		}
	}
}
static int dect_mdm_settings_write_all(struct dect_mdm_settings *dect_sett)
{
	int ret = settings_save_one(DECT_MDM_SETT_TREE_KEY "/" DECT_MDM_SETT_COMMON_CONFIG_KEY,
				    dect_sett, sizeof(struct dect_mdm_settings));

	if (ret) {
		LOG_ERR("Cannot save dect common settings, err: %d", ret);
		return ret;
	}

	settings_data = *dect_sett;

	LOG_INF("dect common settings saved");

	return ret;
}

static uint16_t dect_mdm_settings_write_validate(const struct dect_settings *settings_in)
{
	uint16_t failure_scope_bitmap = 0;

	if (((settings_in->cmd_params.write_scope_bitmap & DECT_SETTINGS_WRITE_SCOPE_IDENTITIES) ||
	     (settings_in->cmd_params.write_scope_bitmap == DECT_SETTINGS_WRITE_SCOPE_ALL)) &&
	    settings_in->identities.transmitter_long_rd_id == DECT_MDM_LONG_RD_ID_ID_NOT_SET) {
		failure_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_IDENTITIES;
	}
	if (((settings_in->cmd_params.write_scope_bitmap & DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE) ||
	     (settings_in->cmd_params.write_scope_bitmap == DECT_SETTINGS_WRITE_SCOPE_ALL)) &&
	    (settings_in->device_type != DECT_DEVICE_TYPE_FT &&
	     settings_in->device_type != DECT_DEVICE_TYPE_PT)) {
		/* Only DECT_DEVICE_TYPE_FT or DECT_DEVICE_TYPE_PT is allowed, not both */
		failure_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE;
	}
	if (((settings_in->cmd_params.write_scope_bitmap & DECT_SETTINGS_WRITE_SCOPE_TX) ||
	     (settings_in->cmd_params.write_scope_bitmap == DECT_SETTINGS_WRITE_SCOPE_ALL)) &&
		dect_mdm_ctrl_utils_tx_pwr_dbm_is_valid_by_band(
		    settings_in->tx.max_power_dbm, settings_in->band_nbr) == false) {
		failure_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_TX;
	}
	if (((settings_in->cmd_params.write_scope_bitmap & DECT_SETTINGS_WRITE_SCOPE_CLUSTER) ||
	     (settings_in->cmd_params.write_scope_bitmap == DECT_SETTINGS_WRITE_SCOPE_ALL)) &&
		dect_mdm_ctrl_utils_tx_pwr_dbm_is_valid_by_band(
		    settings_in->cluster.max_beacon_tx_power_dbm, settings_in->band_nbr) == false) {
		failure_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
	}
	return failure_scope_bitmap;
}

struct dect_mdm_settings_write_status
dect_mdm_settings_write(struct dect_mdm_settings *dect_sett_in)
{
	struct dect_mdm_settings_write_status return_status = {
		.status = 0,
		.reactivate = false,
	};
	struct dect_mdm_settings *current_sett_ptr = dect_mdm_settings_ref_get();
	uint16_t write_scope_bitmap_in =
		dect_sett_in->net_mgmt_common.cmd_params.write_scope_bitmap;
	struct dect_settings *current_sett = &current_sett_ptr->net_mgmt_common;
	struct dect_settings *new_sett = &dect_sett_in->net_mgmt_common;

	/* Some validations done here */
	return_status.failure_scope_bitmap =
		dect_mdm_settings_write_validate(&dect_sett_in->net_mgmt_common);
	if (return_status.failure_scope_bitmap) {
		return_status.status = -EINVAL;
		LOG_ERR("Settings write validation failed, failure bitmap: 0x%04X",
			return_status.failure_scope_bitmap);
		return return_status;
	}

	if (write_scope_bitmap_in == DECT_SETTINGS_WRITE_SCOPE_ALL) {
		return_status.status = dect_mdm_settings_write_all(dect_sett_in);
		if (return_status.status) {
			return return_status;
		}
		return_status.reactivate = true;
		return return_status;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_AUTO_START) {
		current_sett->auto_start.activate = new_sett->auto_start.activate;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_REGION) {
		current_sett->region = new_sett->region;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE) {
		current_sett->device_type = new_sett->device_type;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_IDENTITIES) {
		current_sett->identities.network_id = new_sett->identities.network_id;
		current_sett->identities.transmitter_long_rd_id =
			new_sett->identities.transmitter_long_rd_id;
		return_status.reactivate = true;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_TX) {
		current_sett->tx.max_power_dbm = new_sett->tx.max_power_dbm;
		current_sett->tx.max_mcs = new_sett->tx.max_mcs;
		return_status.reactivate = true;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_POWER_SAVE) {
		current_sett->power_save = new_sett->power_save;
		return_status.reactivate = true;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_BAND_NBR) {
		current_sett->band_nbr = new_sett->band_nbr;
		return_status.reactivate = true;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN) {
		current_sett->rssi_scan.scan_suitable_percent =
			new_sett->rssi_scan.scan_suitable_percent;
		current_sett->rssi_scan.time_per_channel_ms =
			new_sett->rssi_scan.time_per_channel_ms;
		current_sett->rssi_scan.busy_threshold_dbm = new_sett->rssi_scan.busy_threshold_dbm;
		current_sett->rssi_scan.free_threshold_dbm = new_sett->rssi_scan.free_threshold_dbm;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_CLUSTER) {
		current_sett->cluster.beacon_period = new_sett->cluster.beacon_period;
		current_sett->cluster.max_cluster_power_dbm =
			new_sett->cluster.max_cluster_power_dbm;
		current_sett->cluster.max_beacon_tx_power_dbm =
			new_sett->cluster.max_beacon_tx_power_dbm;
		current_sett->cluster.max_num_neighbors =
			new_sett->cluster.max_num_neighbors;
		current_sett->cluster.channel_loaded_percent =
			new_sett->cluster.channel_loaded_percent;
		current_sett->cluster.neighbor_inactivity_disconnect_timer_ms =
			new_sett->cluster.neighbor_inactivity_disconnect_timer_ms;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_NW_BEACON) {
		current_sett->nw_beacon.beacon_period = new_sett->nw_beacon.beacon_period;
		current_sett->nw_beacon.channel = new_sett->nw_beacon.channel;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION) {
		current_sett->association.max_beacon_rx_failures =
			new_sett->association.max_beacon_rx_failures;
		current_sett->association.min_sensitivity_dbm =
			new_sett->association.min_sensitivity_dbm;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN) {
		current_sett->network_join.target_ft_long_rd_id =
			new_sett->network_join.target_ft_long_rd_id;
	}
	if (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION) {
		current_sett->sec_conf.mode = new_sett->sec_conf.mode;
		memcpy(current_sett->sec_conf.integrity_key, new_sett->sec_conf.integrity_key,
		       sizeof(current_sett->sec_conf.integrity_key));
		memcpy(current_sett->sec_conf.cipher_key, new_sett->sec_conf.cipher_key,
		       sizeof(current_sett->sec_conf.cipher_key));
		return_status.reactivate = true;
	}

	return_status.status =
		settings_save_one(DECT_MDM_SETT_TREE_KEY "/" DECT_MDM_SETT_COMMON_CONFIG_KEY,
				  current_sett_ptr, sizeof(struct dect_mdm_settings));
	if (return_status.status) {
		LOG_ERR("Cannot save dect settings, err: %d", return_status.status);
	}

	return return_status;
}

struct dect_mdm_settings_write_status dect_mdm_settings_defaults_set(void)
{
	struct dect_mdm_settings_write_status return_status = {
		.status = 0,
		.reactivate = true,
	};

	return_status.status =
		settings_save_one(DECT_MDM_SETT_TREE_KEY "/" DECT_MDM_SETT_COMMON_CONFIG_KEY,
				  &settings_data_defaults, sizeof(struct dect_mdm_settings));
	if (return_status.status) {
		LOG_ERR("Cannot set dect default settings, err: %d", return_status.status);
		return return_status;
	}
	memcpy(&settings_data, &settings_data_defaults, sizeof(struct dect_mdm_settings));

	dect_mdm_settings_tx_id_default_set();

	LOG_INF("Default settings set.");

	return return_status;
}

int dect_mdm_settings_read(struct dect_mdm_settings *dect_sett)
{
	memcpy(dect_sett, &settings_data, sizeof(struct dect_mdm_settings));
	return 0;
}

struct dect_mdm_settings *dect_mdm_settings_ref_get(void)
{
	return &settings_data;
}

static int dect_mdm_settings_handler(const char *key, size_t len, settings_read_cb read_cb,
				       void *cb_arg)
{
	int ret = 0;

	if (strcmp(key, DECT_MDM_SETT_COMMON_CONFIG_KEY) == 0) {
		ret = read_cb(cb_arg, &settings_data, sizeof(settings_data));
		if (ret < 0) {
			LOG_ERR("Failed to read dect settings_data: %d", ret);
			return ret;
		}
		return 0;
	}
	return -ENOENT;
}

static int dect_mdm_settings_loaded(void)
{
	dect_mdm_settings_tx_id_default_set();

	k_sem_give(&dect_settings_init_sema);
	return 0;
}

static struct settings_handler cfg = {.name = DECT_MDM_SETT_TREE_KEY,
				      .h_set = dect_mdm_settings_handler,
				      .h_commit = dect_mdm_settings_loaded};

int dect_mdm_settings_init(void)
{
	int ret = 0;

	/* Set the initial defaults from the code: */
	settings_data.net_mgmt_common = common_settings_data;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Failed to initialize settings subsystem: %d", ret);
		goto exit;
	}
	ret = settings_register(&cfg);
	if (ret) {
		LOG_ERR("Cannot register settings handler: %d", ret);
		goto exit;
	}
	ret = settings_load_subtree(DECT_MDM_SETT_TREE_KEY);
	if (ret) {
		LOG_ERR("Cannot load settings: %d", ret);
		goto exit;
	}

exit:
	ret = k_sem_take(&dect_settings_init_sema, K_SECONDS(2));
	if (ret) {
		LOG_ERR("dect_mdm_settings_init timeout");
	}

	return ret;
}
