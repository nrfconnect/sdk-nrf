/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_DRIVER_UTILS_H
#define DECT_MDM_DRIVER_UTILS_H

#include <nrf_modem_dect.h>
#include <net/dect/dect_net_l2.h>

#include "dect_mdm_ctrl.h"

/** Buffer size for string conversion functions */
#define DECT_MDM_UTILS_STR_BUFF_SIZE 128

struct dect_mdm_utils_mapping_tbl_item {
	int key;
	char *value_str;
};

/******************************************************************************/

void dect_mdm_utils_modem_mac_err_to_string(enum nrf_modem_dect_mac_err err, char *out_str_buff,
					    size_t buff_size);
void dect_mdm_utils_modem_association_ind_err_to_string(
	enum nrf_modem_dect_mac_association_indication_status err,
	char *out_str_buff, size_t buff_size);
enum dect_status_values
dect_mdm_utils_modem_status_to_net_mgmt_status(enum nrf_modem_dect_mac_err mdm_status);

/******************************************************************************/

bool dect_common_utils_use_harmonized_std(uint8_t band_nbr);
bool dect_common_utils_harmonized_band_channel_array_get(uint8_t band_nbr, uint16_t *channel_array,
							 uint8_t *channel_array_size);
bool dect_common_utils_channel_is_supported_by_band(uint16_t band_nbr, uint16_t channel);

int dect_mdm_utils_mdm_rssi_results_to_l2_rssi_data(
	const struct nrf_modem_dect_mac_rssi_result *rssi_scan_results,
	struct dect_rssi_scan_result_data *rssi_data_out);

bool dect_mdm_utils_cluster_acceptable_for_association(
	struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params *cluster_beacon);

bool dect_mdm_ctrl_utils_tx_pwr_dbm_is_valid_by_band(int8_t tx_pwr_dbm, uint16_t band_nbr);

#endif /* DECT_MDM_DRIVER_UTILS_H */
