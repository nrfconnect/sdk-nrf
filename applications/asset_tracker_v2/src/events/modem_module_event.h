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
#include <zephyr/net/net_ip.h>
#include <modem/lte_lc.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Modem event types submitted by Modem module. */
enum modem_module_event_type {
	/** Event signalling that the modem library and AT command library
	 *  have been initialized and are ready for use by other modules.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_INITIALIZED,

	/** The device has been registered with an LTE network.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_LTE_CONNECTED,

	/** The device has been de-registered with an LTE network.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_LTE_DISCONNECTED,

	/** The device is searching for an LTE network.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_LTE_CONNECTING,

	/** The device is connected to a new LTE cell.
	 *  The event has associated payload of type @ref modem_module_cell in
	 *  the `data.cell` member.
	 */
	MODEM_EVT_LTE_CELL_UPDATE,

	/** PSM configuration has been received.
	 *  The event has associated payload of type @ref modem_module_psm in
	 *  the `data.psm` member.
	 */
	MODEM_EVT_LTE_PSM_UPDATE,

	/** eDRX configuration has been received.
	 *  The event has associated payload of type @ref modem_module_edrx in
	 *  the `data.edrx` member.
	 */
	MODEM_EVT_LTE_EDRX_UPDATE,

	/** Static modem data has been sampled and is ready.
	 *  The event has associated payload of type @ref modem_module_static_modem_data in
	 *  the `data.modem_static` member.
	 */
	MODEM_EVT_MODEM_STATIC_DATA_READY,

	/** Dynamic modem data has been sampled and is ready.
	 *  The event has associated payload of type @ref modem_module_dynamic_modem_data in
	 *  the `data.modem_dynamic` member.
	 */
	MODEM_EVT_MODEM_DYNAMIC_DATA_READY,

	/** Static modem data could not be sampled will not be ready for this
	 *  sampling interval.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_MODEM_STATIC_DATA_NOT_READY,

	/** Dynamic modem data could not be sampled will not be ready for this
	 *  sampling interval.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY,

	/** Battery data could not be sampled will not be ready for this sampling interval.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_BATTERY_DATA_NOT_READY,

	/** The modem module has successfully shut down.
	 *  The event has associated payload of type `uint32_t` in the `data.id` member.
	 */
	MODEM_EVT_SHUTDOWN_READY,

	/** A critical error has occurred, and the application should reboot to recover as
	 *  the module may enter an undefined state.
	 *  The event has associated payload of the type `int` in the `data.err` member.
	 */
	MODEM_EVT_ERROR,

	/** The carrier library has initialized the modem library and it is
	 *  now ready to be used. When the carrier library is enabled, this
	 *  event must be received before the modem module can proceed to initialize
	 *  other dependencies and subsequently send MODEM_EVT_INITIALIZED.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_CARRIER_INITIALIZED,
	/** Due to modem limitations for active TLS connections, the carrier
	 *  library requires all other TLS connections in the system to
	 *  be terminated while FOTA update is ongoing.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_CARRIER_FOTA_PENDING,
	/** FOTA update has been stopped and the application can set up TLS
	 *  connections again.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_CARRIER_FOTA_STOPPED,
	/** The carrier library requests that the device reboots to apply
	 *  downloaded firmware image(s) or for other reasons.
	 *  The event has no associated payload.
	 */
	MODEM_EVT_CARRIER_REBOOT_REQUEST,
	MODEM_EVT_CARRIER_EVENT_LTE_LINK_UP_REQUEST,
	MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST,
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
	char iccid[23];
	char app_version[CONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN];
	char board_version[30];
	char modem_fw[40];
	char imei[16];
};

struct modem_module_dynamic_modem_data {
	int64_t timestamp;
	uint16_t area_code;
	uint32_t cell_id;
	int16_t rsrp;
	uint16_t mcc;
	uint16_t mnc;
	char ip_address[INET6_ADDRSTRLEN];
	char apn[CONFIG_MODEM_APN_LEN_MAX];
	char mccmnc[7];
	uint8_t band;
	enum lte_lc_lte_mode nw_mode;
};

/** @brief Modem event. */
struct modem_module_event {
	struct app_event_header header;
	enum modem_module_event_type type;
	union {
		struct modem_module_static_modem_data modem_static;
		struct modem_module_dynamic_modem_data modem_dynamic;
		struct modem_module_cell cell;
		struct modem_module_psm psm;
		struct modem_module_edrx edrx;
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(modem_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODEM_MODULE_EVENT_H_ */
