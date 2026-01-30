/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing promiscuous mode
 * support functions for the FMAC IF Layer
 * of the Wi-Fi driver.
 */

#include "osal_api.h"
#include "system/fmac_structs.h"
#include "system/fmac_promisc.h"

bool nrf_wifi_util_check_filt_setting(struct nrf_wifi_fmac_vif_ctx *vif,
				      unsigned short *frame_control)
{
	bool filter_check = false;
	struct nrf_wifi_fmac_frame_ctrl *frm_ctrl =
	(struct nrf_wifi_fmac_frame_ctrl *)frame_control;

	switch (vif->packet_filter) {
	case FILTER_MGMT_ONLY:
			if (frm_ctrl->type == NRF_WIFI_MGMT_PKT_TYPE) {
				filter_check = true;
			}
			break;
	case FILTER_DATA_ONLY:
			if (frm_ctrl->type == NRF_WIFI_DATA_PKT_TYPE) {
				filter_check = true;
			}
			break;
	case FILTER_DATA_MGMT:
			if ((frm_ctrl->type == NRF_WIFI_DATA_PKT_TYPE) ||
			    (frm_ctrl->type == NRF_WIFI_MGMT_PKT_TYPE)) {
				filter_check = true;
			}
			break;
	case FILTER_CTRL_ONLY:
			if (frm_ctrl->type == NRF_WIFI_CTRL_PKT_TYPE) {
				filter_check = true;
			}
			break;
	case FILTER_CTRL_MGMT:
			if ((frm_ctrl->type == NRF_WIFI_CTRL_PKT_TYPE) ||
			    (frm_ctrl->type == NRF_WIFI_MGMT_PKT_TYPE)) {
				filter_check = true;
			}
			break;
	case FILTER_DATA_CTRL:
			if ((frm_ctrl->type == NRF_WIFI_DATA_PKT_TYPE) ||
			    (frm_ctrl->type == NRF_WIFI_CTRL_PKT_TYPE)) {
				filter_check = true;
			}
			break;
		/* all other packet_filter cases - bit 0 is set and hence all
		 * packet types are allowed or in the case of 0xE - all packet
		 * type bits are enabled
		 */
	default:
			filter_check = true;
			break;
	}
	return filter_check;
}
