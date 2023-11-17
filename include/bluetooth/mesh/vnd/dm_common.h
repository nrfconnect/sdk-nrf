/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_dm Distance Measurement Models
 * @{
 * @brief Common API for the Distance Measurement models.
 */

#ifndef BT_MESH_DM_H__
#define BT_MESH_DM_H__

#include <bluetooth/mesh/model_types.h>
#include <dm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MESH_VENDOR_COMPANY_ID 0x0059
#define BT_MESH_MODEL_ID_DM_SRV 0x000B
#define BT_MESH_MODEL_ID_DM_CLI 0x000C

/** Parameters for the Distance Measurement Result entry. */
struct bt_mesh_dm_res_entry {
	/** Mode used for ranging. */
	enum dm_ranging_mode mode;
	/** Quality indicator. */
	enum dm_quality quality;
	/** Indicator if an error occurred. */
	bool err_occurred;
	/** Unicast address of the node distance was measured with. */
	uint16_t addr;
	/** Container of distance estimate results for a number
	 * of different methods, expressed in centimeters.
	 */
	union {
		/** Distance estimate based on RTT measurement. */
		uint16_t rtt;
		/** Multi-carrier phase difference measurement. */
		struct {
			/** Distance estimate based on IFFT of spectrum. */
			uint16_t best;
			/** Distance estimate based on average phase slope estimation. */
			uint16_t ifft;
			/** Distance estimate based on Friis path loss formula. */
			uint16_t phase_slope;
			/** Best effort distance estimate. */
			uint16_t rssi_openspace;
		} mcpd;
	} res;
};

/** Parameters for the Distance Measurement Config. */
struct bt_mesh_dm_cfg {
	/** Time to live for sync message. */
	uint8_t ttl;
	/** Timeout for distance measurement procedure in 100 ms per step. */
	uint8_t timeout;
	/** Reflector start delay. */
	uint8_t delay;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_DM_SRV_OP BT_MESH_MODEL_OP_3(0x0F, BT_MESH_VENDOR_COMPANY_ID)
#define BT_MESH_DM_CLI_OP BT_MESH_MODEL_OP_3(0x10, BT_MESH_VENDOR_COMPANY_ID)

#define BT_MESH_DM_CONFIG_OP 0x01
#define BT_MESH_DM_CONFIG_STATUS_OP 0x02
#define BT_MESH_DM_START_OP 0x03
#define BT_MESH_DM_RESULT_GET_OP 0x04
#define BT_MESH_DM_RESULT_STATUS_OP 0x05
#define BT_MESH_DM_SYNC_OP 0x06

#define BT_MESH_DM_CONFIG_MSG_LEN_MIN 1
#define BT_MESH_DM_CONFIG_MSG_LEN_MAX 4
#define BT_MESH_DM_CONFIG_STATUS_MSG_LEN 6
#define BT_MESH_DM_START_MSG_LEN_MIN 4
#define BT_MESH_DM_START_MSG_LEN_MAX 7
#define BT_MESH_DM_RESULT_GET_MSG_LEN 2
#define BT_MESH_DM_RESULT_STATUS_MSG_MIN_LEN 2
#define BT_MESH_DM_RESULT_STATUS_MSG_MAX_LEN 100
#define BT_MESH_DM_SYNC_MSG_LEN 3
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_DM_H__ */

/** @} */
