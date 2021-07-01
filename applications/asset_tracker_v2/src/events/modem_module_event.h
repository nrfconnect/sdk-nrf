/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODEM_MODULE_EVENT_H_
#define _MODEM_MODULE_EVENT_H_

/**
 * @brief Modem module event
 * @defgroup modem_module_event Modem module event
 * @{
 */
#include <net/net_ip.h>
#include <modem/lte_lc.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Modem event types submitted by Modem module. */
enum modem_module_event_type {
	MODEM_EVT_LTE_CONNECTED,
	MODEM_EVT_LTE_DISCONNECTED,
	MODEM_EVT_LTE_CONNECTING,
	MODEM_EVT_LTE_CELL_UPDATE,
	MODEM_EVT_LTE_PSM_UPDATE,
	MODEM_EVT_LTE_EDRX_UPDATE,
	MODEM_EVT_MODEM_STATIC_DATA_READY,
	MODEM_EVT_MODEM_DYNAMIC_DATA_READY,
	MODEM_EVT_MODEM_STATIC_DATA_NOT_READY,
	MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY,
	MODEM_EVT_NEIGHBOR_CELLS_DATA_READY,
	MODEM_EVT_NEIGHBOR_CELLS_DATA_NOT_READY,
	MODEM_EVT_BATTERY_DATA_NOT_READY,
	MODEM_EVT_BATTERY_DATA_READY,
	MODEM_EVT_SHUTDOWN_READY,
	MODEM_EVT_ERROR
};

/** @brief LTE cell information. */
struct modem_module_cell {
	/** E-UTRAN cell ID. */
	uint32_t cell_id;
	/** Tracking area code. */
	uint32_t tac;
};

/** @brief PSM information. */
struct modem_module_psm {
	/** Tracking Area Update interval [s]. -1 if the timer is disabled. */
	int tau;
	/** Active time [s]. -1 if the timer is disabled. */
	int active_time;
};

/** @brief eDRX information. */
struct modem_module_edrx {
	/** eDRX interval value [s] */
	float edrx;
	/** Paging time window [s] */
	float ptw;
};

struct modem_module_static_modem_data {
	int64_t timestamp;
	uint16_t band;
	uint16_t nw_mode_gps;
	uint16_t nw_mode_ltem;
	uint16_t nw_mode_nbiot;
	char iccid[23];
	char app_version[CONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN];
	char board_version[30];
	char modem_fw[40];
};

struct modem_module_dynamic_modem_data {
	int64_t timestamp;
	uint16_t area_code;
	uint32_t cell_id;
	int16_t rsrp;
	char ip_address[INET6_ADDRSTRLEN];
	char mccmnc[7];

	/* Flags to signify if the corresponding data value has been updated and is concidered
	 * fresh.
	 */
	bool area_code_fresh	: 1;
	bool cell_id_fresh	: 1;
	bool rsrp_fresh		: 1;
	bool ip_address_fresh	: 1;
	bool mccmnc_fresh	: 1;
};

struct modem_module_battery_data {
	uint16_t battery_voltage;
	int64_t timestamp;
};

struct modem_module_neighbor_cells {
	struct lte_lc_cells_info cell_data;
	struct lte_lc_ncell neighbor_cells[17];
	int64_t timestamp;
};

/** @brief Modem event. */
struct modem_module_event {
	struct event_header header;
	enum modem_module_event_type type;
	union {
		struct modem_module_static_modem_data modem_static;
		struct modem_module_dynamic_modem_data modem_dynamic;
		struct modem_module_battery_data bat;
		struct modem_module_cell cell;
		struct modem_module_psm psm;
		struct modem_module_edrx edrx;
		struct modem_module_neighbor_cells neighbor_cells;
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		int err;
	} data;
};

EVENT_TYPE_DECLARE(modem_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODEM_MODULE_EVENT_H_ */
