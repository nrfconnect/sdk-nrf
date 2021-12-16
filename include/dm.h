/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DM_H_
#define DM_H_

/**@file
 * @defgroup dm Distance Measurement API
 * @{
 * @brief API for the Distance Measurement (DM).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <zephyr/types.h>
#include <bluetooth/addr.h>

/** @brief Role definition. */
enum dm_dev_role {
	/* Undefined role*/
	DM_ROLE_NONE,

	/* Act as a initiator */
	DM_ROLE_INITIATOR,

	/* Act as a reflector */
	DM_ROLE_REFLECTOR,
};

/** @brief Ranging mode definition. */
enum dm_ranging_mode {
	/* Round trip timing */
	DM_RANGING_MODE_RTT,

	/* Multi-carrier phase difference */
	DM_RANGING_MODE_MCPD,
};

/** @brief Measurement quality definition. */
enum dm_quality {
	/* Good measurement quality */
	DM_QUALITY_OK,

	/* Poor measurement quality */
	DM_QUALITY_POOR,

	/* Measurement not for use */
	DM_QUALITY_DO_NOT_USE,

	/* Incorrect CRC measurement */
	DM_QUALITY_CRC_FAIL,

	/* Measurement quality not specified */
	DM_QUALITY_NONE,
};

/** @brief Measurement structure. */
struct dm_result {
	/* Status of the precedure. \a true if measurement was done correctly, \a false otherwise */
	bool status;

	/* Quality indicator */
	enum dm_quality quality;

	/* Bluetooth LE Device Address */
	bt_addr_le_t bt_addr;

	/* Mode used for ranging */
	enum dm_ranging_mode ranging_mode;
	union {
		struct mcpd {
			/* MCPD: Distance estimate based on IFFT of spectrum */
			float ifft;

			/* MCPD: Distance estimate based on average phase slope estimation */
			float phase_slope;

			/* RSSI: Distance estimate based on Friis path loss formula */
			float rssi_openspace;

			/* Best effort distance estimate */
			float best;
		} mcpd;
		struct rtt {
			/* RTT: Distance estimate based on RTT measurement */
			float rtt;
		} rtt;
	} dist_estimates;
};

/** @brief Event callback structure. */
struct dm_cb {
	/** @brief Data ready.
	 *
	 *  @param result Measurement data.
	 */
	void (*data_ready)(struct dm_result *result);
};

/** @brief DM initialization parameters. */
struct dm_init_param {
	/* Event callback structure. */
	struct dm_cb *cb;
};

/** @brief DM request structure */
struct dm_request {
	/* Role of the device. */
	enum dm_dev_role role;

	/* Bluetooth LE Device Address */
	bt_addr_le_t bt_addr;

	/* Access address used for packet exchanges. */
	uint32_t access_address;

	/* Ranging mode to use in the procedure. */
	enum dm_ranging_mode ranging_mode;

	/* Start delay */
	uint32_t start_delay_us;
};

/** @brief Initialize the DM.
 *
 *  Initialize the DM by specifying a list of supported operations.
 *
 *  @param init_param Initialization parameters.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int dm_init(struct dm_init_param *init_param);

/** @brief Add measurement request.
 *
 *  Adding a measurement request. This is related to timeslot allocation.
 *
 *  @param req Address of the structure with request parameters.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int dm_request_add(struct dm_request *req);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* DM_H_ */
