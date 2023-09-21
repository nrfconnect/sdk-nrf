/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef LTE_LC_H__
#define LTE_LC_H__

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file lte_lc.h
 *
 * @defgroup lte_lc LTE link control library
 *
 * @{
 *
 * Public APIs for the LTE link control library.
 */

/**
 * Network registration status.
 *
 * @note Maps directly to the registration status as returned by the AT command `AT+CEREG?`.
 */
enum lte_lc_nw_reg_status {
	/** Not registered. UE is not currently searching for an operator to register to. */
	LTE_LC_NW_REG_NOT_REGISTERED		= 0,

	/** Registered, home network. */
	LTE_LC_NW_REG_REGISTERED_HOME		= 1,

	/**
	 * Not registered, but UE is currently trying to attach or searching for an operator to
	 * register to.
	 */
	LTE_LC_NW_REG_SEARCHING			= 2,

	/** Registration denied. */
	LTE_LC_NW_REG_REGISTRATION_DENIED	= 3,

	/** Unknown, for example out of LTE coverage. */
	LTE_LC_NW_REG_UNKNOWN			= 4,

	/** Registered, roaming. */
	LTE_LC_NW_REG_REGISTERED_ROAMING	= 5,

	/** Not registered due to UICC failure. */
	LTE_LC_NW_REG_UICC_FAIL			= 90
};

/** System mode. */
enum lte_lc_system_mode {
	/** LTE-M only. */
	LTE_LC_SYSTEM_MODE_LTEM = 1,

	/** NB-IoT only. */
	LTE_LC_SYSTEM_MODE_NBIOT,

	/** GNSS only. */
	LTE_LC_SYSTEM_MODE_GPS,

	/** LTE-M + GNSS. */
	LTE_LC_SYSTEM_MODE_LTEM_GPS,

	/** NB-IoT + GNSS. */
	LTE_LC_SYSTEM_MODE_NBIOT_GPS,

	/** LTE-M + NB-IoT. */
	LTE_LC_SYSTEM_MODE_LTEM_NBIOT,

	/** LTE-M + NB-IoT + GNSS. */
	LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS,
};

/**
 * LTE mode.
 *
 * The values for LTE-M and NB-IoT correspond to the values for the access technology field in AT
 * responses.
 */
enum lte_lc_lte_mode {
	/** None. */
	LTE_LC_LTE_MODE_NONE	= 0,

	/** LTE-M. */
	LTE_LC_LTE_MODE_LTEM	= 7,

	/** NB-IoT. */
	LTE_LC_LTE_MODE_NBIOT	= 9,
};

/**
 * LTE mode preference.
 *
 * If multiple LTE system modes are enabled, the modem can select the mode that best meets the
 * criteria set by this configuration. The LTE mode preference does not affect the way GNSS
 * operates.
 *
 * Note that there's a distinction between preferred and prioritized mode.
 */
enum lte_lc_system_mode_preference {
	/** No LTE preference, automatically selected by the modem. */
	LTE_LC_SYSTEM_MODE_PREFER_AUTO = 0,

	/**
	 * LTE-M is preferred over PLMN selection.
	 *
	 * The modem will prioritize to use LTE-M and switch over to a PLMN where LTE-M is available
	 * whenever possible.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_LTEM,

	/**
	 * NB-IoT is preferred over PLMN selection.
	 *
	 * The modem will prioritize to use NB-IoT and switch over to a PLMN where NB-IoT is
	 * available whenever possible.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_NBIOT,

	/**
	 * LTE-M is preferred, but PLMN selection is more important.
	 *
	 * The modem will prioritize to stay on home network and switch over to NB-IoT if LTE-M is
	 * not available.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO,

	/**
	 * NB-IoT is preferred, but PLMN selection is more important.
	 *
	 * The modem will prioritize to stay on home network and switch over to LTE-M if NB-IoT is
	 * not available.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO
};

/**
 * Functional mode, used to control RF functionality in the modem.
 *
 * @note The values are mapped to modem functional modes as described in
 *       "nRF91 AT Commands - Command Reference Guide". Refer to the AT command guide to
 *       verify whether a functional mode is supported by a given modem firmware version.
 */
enum lte_lc_func_mode {
	/**
	 * Sets the device to minimum functionality.
	 *
	 * Disables both transmit and receive RF circuits and deactivates LTE and GNSS.
	 */
	LTE_LC_FUNC_MODE_POWER_OFF		= 0,

	/**
	 * Sets the device to full functionality.
	 *
	 * Both LTE and GNSS will become active if the respective system modes are enabled.
	 */
	LTE_LC_FUNC_MODE_NORMAL			= 1,

	/**
	 * Sets the device to receive only functionality.
	 *
	 * Can be used, for example, to evaluate connections with
	 * lte_lc_conn_eval_params_get().
	 *
	 * @note This mode is only supported by modem firmware versions >= 1.3.0.
	 */
	LTE_LC_FUNC_MODE_RX_ONLY		= 2,

	/**
	 * Sets the device to flight mode.
	 *
	 * Disables both transmit and receive RF circuits and deactivates LTE and GNSS services.
	 */
	LTE_LC_FUNC_MODE_OFFLINE		= 4,

	/** Deactivates LTE without shutting down GNSS services. */
	LTE_LC_FUNC_MODE_DEACTIVATE_LTE		= 20,

	/** Activates LTE without changing GNSS. */
	LTE_LC_FUNC_MODE_ACTIVATE_LTE		= 21,

	/** Deactivates GNSS without shutting down LTE services. */
	LTE_LC_FUNC_MODE_DEACTIVATE_GNSS	= 30,

	/** Activates GNSS without changing LTE. */
	LTE_LC_FUNC_MODE_ACTIVATE_GNSS		= 31,

	/** Deactivates UICC. */
	LTE_LC_FUNC_MODE_DEACTIVATE_UICC	= 40,

	/** Activates UICC. */
	LTE_LC_FUNC_MODE_ACTIVATE_UICC		= 41,

	/** Sets the device to flight mode without shutting down UICC. */
	LTE_LC_FUNC_MODE_OFFLINE_UICC_ON	= 44,
};

/** Event type. */
enum lte_lc_evt_type {
	/**
	 * Network registration status.
	 *
	 * The associated payload is the @c nw_reg_status member of type @ref lte_lc_nw_reg_status
	 * in the event.
	 */
	LTE_LC_EVT_NW_REG_STATUS,

	/**
	 * PSM parameters provided by the network.
	 *
	 * The associated payload is the @c psm_cfg member of type @ref lte_lc_psm_cfg in the event.
	 */
	LTE_LC_EVT_PSM_UPDATE,

	/**
	 * eDRX parameters provided by the network.
	 *
	 * The associated payload is the @c edrx_cfg member of type @ref lte_lc_edrx_cfg in the
	 * event.
	 */
	LTE_LC_EVT_EDRX_UPDATE,

	/**
	 * RRC connection state.
	 *
	 * The associated payload is the @c rrc_mode member of type @ref lte_lc_rrc_mode in the
	 * event.
	 */
	LTE_LC_EVT_RRC_UPDATE,

	/**
	 * Current cell.
	 *
	 * The associated payload is the @c cell member of type @ref lte_lc_cell in the event.
	 * Only the @c cell.tac and @c cell.id members are populated in the event payload. The rest
	 * are expected to be zero.
	 */
	LTE_LC_EVT_CELL_UPDATE,

	/**
	 * Current LTE mode.
	 *
	 * If a system mode that enables both LTE-M and NB-IoT is configured, the modem may change
	 * the currently active LTE mode based on the system mode preference and network
	 * availability. This event will then indicate which LTE mode is currently used by the
	 * modem.
	 *
	 * The associated payload is the @c lte_mode member of type @ref lte_lc_lte_mode in the
	 * event.
	 */
	LTE_LC_EVT_LTE_MODE_UPDATE,

	/**
	 * Tracking Area Update pre-warning.
	 *
	 * This event will be received some time before TAU is scheduled to occur. The time is
	 * configurable. This gives the application the opportunity to send data over the network
	 * before the TAU happens, thus saving power by avoiding sending data and the TAU
	 * separately.
	 *
	 * The associated payload is the @c time member of type @c uint64_t in the event.
	 */
	LTE_LC_EVT_TAU_PRE_WARNING,

	/**
	 * Neighbor cell measurement results.
	 *
	 * The associated payload is the @c cells_info member of type @ref lte_lc_cells_info in the
	 * event.
	 */
	LTE_LC_EVT_NEIGHBOR_CELL_MEAS,

	/**
	 * Modem sleep pre-warning.
	 *
	 * This event will be received some time before the modem exits sleep. The time is
	 * configurable.
	 *
	 * The associated payload is the @c modem_sleep member of type @ref lte_lc_modem_sleep in
	 * the event. The @c time parameter indicates the time until modem exits sleep.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING,

	/**
	 * Modem exited from sleep.
	 *
	 * The associated payload is the @c modem_sleep member of type @ref lte_lc_modem_sleep in
	 * the event.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT,

	/**
	 * Modem entered sleep.
	 *
	 * The associated payload is the @c modem_sleep member of type @ref lte_lc_modem_sleep in
	 * the event. The @c time parameter indicates the duration of the sleep.
	 */
	LTE_LC_EVT_MODEM_SLEEP_ENTER,

	/**
	 * Information about modem operation.
	 *
	 * The associated payload is the @c modem_evt member of type @ref lte_lc_modem_evt in the
	 * event.
	 */
	LTE_LC_EVT_MODEM_EVENT,
};

/** RRC connection state. */
enum lte_lc_rrc_mode {
	/** Idle. */
	LTE_LC_RRC_MODE_IDLE		= 0,

	/** Connected. */
	LTE_LC_RRC_MODE_CONNECTED	= 1,
};

/** Power Saving Mode (PSM) configuration. */
struct lte_lc_psm_cfg {
	/** Periodic Tracking Area Update interval in seconds. */
	int tau;

	/** Active-time (time from RRC idle to PSM) in seconds. */
	int active_time;
};

/** eDRX configuration. */
struct lte_lc_edrx_cfg {
	/**
	 * LTE mode for which the configuration is valid.
	 *
	 * If the mode is @ref LTE_LC_LTE_MODE_NONE, access technology is not using eDRX.
	 */
	enum lte_lc_lte_mode mode;

	/** eDRX interval in seconds. */
	float edrx;

	/** Paging time window in seconds. */
	float ptw;
};

/** Maximum timing advance value. */
#define LTE_LC_CELL_TIMING_ADVANCE_MAX		20512
/** Invalid timing advance value. */
#define LTE_LC_CELL_TIMING_ADVANCE_INVALID	65535
/** Maximum EARFCN value. */
#define LTE_LC_CELL_EARFCN_MAX			262143
/** Unknown or undetectable RSRP value. */
#define LTE_LC_CELL_RSRP_INVALID		255
/** Unknown or undetectable RSRQ value. */
#define LTE_LC_CELL_RSRQ_INVALID		255
/** Cell ID value not valid. */
#define LTE_LC_CELL_EUTRAN_ID_INVALID		UINT32_MAX
/** Maximum cell ID value. */
#define LTE_LC_CELL_EUTRAN_ID_MAX		268435455
/** Time difference not valid. */
#define LTE_LC_CELL_TIME_DIFF_INVALID		0

/** Neighbor cell information. */
struct lte_lc_ncell {
	/** EARFCN per 3GPP TS 36.101. */
	uint32_t earfcn;

	/**
	 * Difference of current cell and neighbor cell measurement, in the range
	 * -99999 ms < time_diff < 99999 ms. @ref LTE_LC_CELL_TIME_DIFF_INVALID if the value is not
	 * valid.
	 */
	int time_diff;

	/** Physical cell ID. */
	uint16_t phys_cell_id;

	/**
	 * RSRP.
	 *
	 * Format is the same as for @c rsrp member of struct @ref lte_lc_cell.
	 */
	int16_t rsrp;

	/**
	 * RSRQ.
	 *
	 * Format is the same as for @c rsrq member of struct @ref lte_lc_cell.
	 */
	int16_t rsrq;
};

/** Cell information. */
struct lte_lc_cell {
	/** Mobile Country Code. */
	int mcc;

	/** Mobile Network Code. */
	int mnc;

	/** E-UTRAN cell ID, range 0 - @ref LTE_LC_CELL_EUTRAN_ID_MAX. */
	uint32_t id;

	/** Tracking area code. */
	uint32_t tac;

	/** EARFCN per 3GPP TS 36.101. */
	uint32_t earfcn;

	/**
	 * Timing advance decimal value.
	 *
	 * Range 0 - @ref LTE_LC_CELL_TIMING_ADVANCE_MAX. @ref LTE_LC_CELL_TIMING_ADVANCE_INVALID if
	 * timing advance is not valid.
	 *
	 * @note Timing advance may be reported from past measurements. The parameters
	 *       @c timing_advance_meas_time and @c measurement_time can be used to evaluate if
	 *       the parameter is usable.
	 */
	uint16_t timing_advance;

	/**
	 * Timing advance measurement time, calculated from modem boot time.
	 *
	 * Range 0 - 18 446 744 073 709 551 614 ms.
	 *
	 * @note For modem firmware versions >= 1.3.1, timing advance measurement time may be
	 *       reported from the modem. This means that timing advance data may now also be
	 *       available in neighbor cell measurements done in RRC idle, even though the timing
	 *       advance data was captured in RRC connected. If the value is not reported by the
	 *       modem, it is set to 0.
	 */
	uint64_t timing_advance_meas_time;

	/**
	 * Measurement time.
	 *
	 * Range 0 - 18 446 744 073 709 551 614 ms.
	 */
	uint64_t measurement_time;

	/** Physical cell ID. */
	uint16_t phys_cell_id;

	/**
	 * RSRP.
	 *
	 * * -17: RSRP < -156 dBm
	 * * -16: -156 ≤ RSRP < -155 dBm
	 * * ...
	 * * -3: -143 ≤ RSRP < -142 dBm
	 * * -2: -142 ≤ RSRP < -141 dBm
	 * * -1: -141 ≤ RSRP < -140 dBm
	 * * 0: RSRP < -140 dBm
	 * * 1: -140 ≤ RSRP < -139 dBm
	 * * 2: -139 ≤ RSRP < -138 dBm
	 * * ...
	 * * 95: -46 ≤ RSRP < -45 dBm
	 * * 96: -45 ≤ RSRP < -44 dBm
	 * * 97: -44 ≤ RSRP dBm
	 * * @ref LTE_LC_CELL_RSRP_INVALID : not known or not detectable
	 */
	int16_t rsrp;

	/**
	 * RSRQ.
	 *
	 * * -30: RSRQ < -34 dB
	 * * -29: -34 ≤ RSRQ < -33.5 dB
	 * * ...
	 * * -2: -20.5 ≤ RSRQ < -20 dB
	 * * -1: -20 ≤ RSRQ < -19.5 dB
	 * * 0: RSRQ < -19.5 dB
	 * * 1: -19.5 ≤ RSRQ < -19 dB
	 * * 2: -19 ≤ RSRQ < -18.5 dB
	 * * ...
	 * * 32: -4 ≤ RSRQ < -3.5 dB
	 * * 33: -3.5 ≤ RSRQ < -3 dB
	 * * 34: -3 ≤ RSRQ dB
	 * * 35: -3 ≤ RSRQ < -2.5 dB
	 * * 36: -2.5 ≤ RSRQ < -2 dB
	 * * ...
	 * * 45: 2 ≤ RSRQ < 2.5 dB
	 * * 46: 2.5 ≤ RSRQ dB
	 * * @ref LTE_LC_CELL_RSRQ_INVALID : not known or not detectable.
	 */
	int16_t rsrq;
};


/** Results of neighbor cell measurements. */
struct lte_lc_cells_info {
	/**
	 * The current cell information is valid if the current cell ID is not set to
	 * @ref LTE_LC_CELL_EUTRAN_ID_INVALID.
	 */
	struct lte_lc_cell current_cell;

	/**
	 * Indicates whether or not the @c neighbor_cells contains valid neighbor cell information.
	 * If it is zero, no neighbor cells were found for the current cell.
	 */
	uint8_t ncells_count;

	/** Neighbor cells for the current cell. */
	struct lte_lc_ncell *neighbor_cells;

	/**
	 * Indicates whether or not the @c gci_cells contains valid surrounding cell information
	 * from GCI search types (@ref lte_lc_neighbor_search_type). If it is zero, no cells were
	 * found.
	 */
	uint8_t gci_cells_count;

	/** Surrounding cells found by the GCI search types. */
	struct lte_lc_cell *gci_cells;
};

/** Modem sleep type. */
enum lte_lc_modem_sleep_type {
	/** Power Saving Mode (PSM). */
	LTE_LC_MODEM_SLEEP_PSM			= 1,

	/** RF inactivity, for example eDRX. */
	LTE_LC_MODEM_SLEEP_RF_INACTIVITY	= 2,

	/** Limited service or out of coverage. */
	LTE_LC_MODEM_SLEEP_LIMITED_SERVICE	= 3,

	/** Flight mode. */
	LTE_LC_MODEM_SLEEP_FLIGHT_MODE		= 4,

	/**
	 * Proprietary PSM.
	 *
	 * @note This type is only supported by modem firmware versions >= 2.0.0.
	 */
	LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM	= 7,
};

/** Modem sleep information. */
struct lte_lc_modem_sleep {
	/** Sleep type. */
	enum lte_lc_modem_sleep_type type;

	/**
	 * Sleep time in milliseconds. If this value is set to -1, the sleep is considered infinite.
	 */
	int64_t time;
};

/** Energy consumption estimate. */
enum lte_lc_energy_estimate {
	/**
	 * Bad conditions.
	 *
	 * Difficulties in setting up connections. Maximum number of repetitions might be needed for
	 * data.
	 */
	LTE_LC_ENERGY_CONSUMPTION_EXCESSIVE	= 5,

	/**
	 * Poor conditions.
	 *
	 * Setting up a connection might require retries and a higher number of repetitions for
	 * data.
	 */
	LTE_LC_ENERGY_CONSUMPTION_INCREASED	= 6,

	/**
	 * Normal conditions.
	 *
	 * No repetitions for data or only a few repetitions in the worst case.
	 */
	LTE_LC_ENERGY_CONSUMPTION_NORMAL	= 7,

	/**
	 * Good conditions.
	 *
	 * Possibly very good conditions for small amounts of data.
	 */
	LTE_LC_ENERGY_CONSUMPTION_REDUCED	= 8,

	/**
	 * Excellent conditions.
	 *
	 * Efficient data transfer estimated also for larger amounts of data.
	 */
	LTE_LC_ENERGY_CONSUMPTION_EFFICIENT	= 9,
};

/** Cell in Tracking Area Identifier list. */
enum lte_lc_tau_triggered {
	/**
	 * The evaluated cell is in the Tracking Area Identifier list.
	 *
	 * When switching to this cell, a TAU will not be triggered.
	 */
	LTE_LC_CELL_IN_TAI_LIST		= 0,

	/**
	 * The evaluated cell is not in the Tracking Area Identifier list.
	 *
	 * When switching to this cell, a TAU will be triggered.
	 */
	LTE_LC_CELL_NOT_IN_TAI_LIST	= 1,

	/** Not known. */
	LTE_LC_CELL_UNKNOWN		= UINT8_MAX
};

/** CE level. */
enum lte_lc_ce_level {
	/** No repetitions or a small number of repetitions. */
	LTE_LC_CE_LEVEL_0	= 0,

	/** Medium number of repetitions. */
	LTE_LC_CE_LEVEL_1	= 1,

	/** Large number of repetitions. */
	LTE_LC_CE_LEVEL_2	= 2,

	/** Very large number of repetitions. */
	LTE_LC_CE_LEVEL_3	= 3,

	/** Not known. */
	LTE_LC_CE_LEVEL_UNKNOWN	= UINT8_MAX,
};

/** Reduced mobility mode. */
enum lte_lc_reduced_mobility_mode {
	/** Functionality according to the 3GPP relaxed monitoring feature. */
	LTE_LC_REDUCED_MOBILITY_DEFAULT = 0,

	/** Enable Nordic-proprietary reduced mobility feature. */
	LTE_LC_REDUCED_MOBILITY_NORDIC = 1,

	/**
	 * Full measurements for best possible mobility.
	 *
	 * Disable the 3GPP relaxed monitoring and Nordic-proprietary reduced mobility features.
	 */
	LTE_LC_REDUCED_MOBILITY_DISABLED = 2,
};

/** Modem domain events. */
enum lte_lc_modem_evt {
	/**
	 * Indicates that a light search has been performed.
	 *
	 * This event gives the application the chance to stop using more power when searching
	 * networks in possibly weaker radio conditions. Before sending this event, the modem
	 * searches for cells based on previous cell history, measures the radio conditions, and
	 * makes assumptions on where networks might be deployed. This event means that the modem
	 * has not found a network that it could select based on the 3GPP network selection rules
	 * from those locations. It does not mean that there are no networks to be found in the
	 * area. The modem continues more thorough searches automatically after sending this status.
	 */
	LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE,

	/**
	 * Indicates that a network search has been performed.
	 *
	 * The modem has found a network that it can select according to the 3GPP network selection
	 * rules or all frequencies have been scanned and a suitable cell was not found. In the
	 * latter case, the modem enters normal limited service state functionality and performs
	 * scan for service periodically.
	 */
	LTE_LC_MODEM_EVT_SEARCH_DONE,

	/**
	 * The modem has detected a reset loop.
	 *
	 * A reset loop indicates that the modem restricts network attach attempts for the next 30
	 * minutes. The timer does not run when the modem has no power or while it stays in the
	 * reset loop. The modem counts all the resets where the modem is not gracefully
	 * deinitialized with `AT+CFUN=0` or using an API performing the equivalent operation, such
	 * as setting the functional mode to @ref LTE_LC_FUNC_MODE_POWER_OFF.
	 */
	LTE_LC_MODEM_EVT_RESET_LOOP,

	/** Battery voltage is low and the modem is therefore deactivated. */
	LTE_LC_MODEM_EVT_BATTERY_LOW,

	/** The device is overheated and the modem is therefore deactivated. */
	LTE_LC_MODEM_EVT_OVERHEATED,

	/** The modem does not have an IMEI. */
	LTE_LC_MODEM_EVT_NO_IMEI,

	/**
	 * Selected CE level in RACH procedure is 0, see 3GPP TS 36.331 for details.
	 *
	 * @note This event is only supported by modem firmware versions >= 2.0.0.
	 */
	LTE_LC_MODEM_EVT_CE_LEVEL_0,

	/**
	 * Selected CE level in RACH procedure is 1, see 3GPP TS 36.331 for details.
	 *
	 * @note This event is only supported by modem firmware versions >= 2.0.0.
	 */
	LTE_LC_MODEM_EVT_CE_LEVEL_1,

	/**
	 * Selected CE level in RACH procedure is 2, see 3GPP TS 36.331 for details.
	 *
	 * @note This event is only supported by modem firmware versions >= 2.0.0.
	 */
	LTE_LC_MODEM_EVT_CE_LEVEL_2,

	/**
	 * Selected CE level in RACH procedure is 3, see 3GPP TS 36.331 for details.
	 *
	 * @note This event is only supported by modem firmware versions >= 2.0.0.
	 */
	LTE_LC_MODEM_EVT_CE_LEVEL_3,
};

/** Type of factory reset to perform. */
enum lte_lc_factory_reset_type {
	/** Reset all modem data to factory settings. */
	LTE_LC_FACTORY_RESET_ALL = 0,

	/** Reset user-configurable data to factory settings. */
	LTE_LC_FACTORY_RESET_USER = 1,
};

/**
 * Connection evaluation parameters.
 *
 * @note For more information on the various connection evaluation parameters, refer to the
 *       "nRF91 AT Commands - Command Reference Guide".
 */
struct lte_lc_conn_eval_params {
	/** RRC connection state during measurements. */
	enum lte_lc_rrc_mode rrc_state;

	/**
	 * Relative estimated energy consumption for data transmission compared to nominal
	 * consumption.
	 */
	enum lte_lc_energy_estimate energy_estimate;

	/**
	 * Value that indicates if the evaluated cell is a part of the Tracking Area Identifier
	 * list received from the network.
	 */
	enum lte_lc_tau_triggered tau_trig;

	/**
	 * Coverage Enhancement level for PRACH.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_IDLE, the CE level is estimated based on RSRP and
	 * network configuration.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_CONNECTED, the currently used CE level is
	 * reported.
	 *
	 * CE levels 0-2 are supported in NB-IoT.
	 *
	 * CE levels 0-1 are supported in LTE-M.
	 */
	enum lte_lc_ce_level ce_level;

	/** EARFCN for given cell where EARFCN is per 3GPP TS 36.101. */
	int earfcn;

	/** Reduction in power density in dB. */
	int16_t dl_pathloss;

	/**
	 * Current RSRP level at time of report.
	 *
	 * * -17: RSRP < -156 dBm
	 * * -16: -156 ≤ RSRP < -155 dBm
	 * * ...
	 * * -3: -143 ≤ RSRP < -142 dBm
	 * * -2: -142 ≤ RSRP < -141 dBm
	 * * -1: -141 ≤ RSRP < -140 dBm
	 * * 0: RSRP < -140 dBm
	 * * 1: -140 ≤ RSRP < -139 dBm
	 * * 2: -139 ≤ RSRP < -138 dBm
	 * * ...
	 * * 95: -46 ≤ RSRP < -45 dBm
	 * * 96: -45 ≤ RSRP < -44 dBm
	 * * 97: -44 ≤ RSRP dBm
	 * * @ref LTE_LC_CELL_RSRP_INVALID : not known or not detectable
	 */
	int16_t rsrp;

	/**
	 * Current RSRQ level at time of report.
	 *
	 * * -30: RSRQ < -34 dB
	 * * -29: -34 ≤ RSRQ < -33.5 dB
	 * * ...
	 * * -2: -20.5 ≤ RSRQ < -20 dB
	 * * -1: -20 ≤ RSRQ < -19.5 dB
	 * * 0: RSRQ < -19.5 dB
	 * * 1: -19.5 ≤ RSRQ < -19 dB
	 * * 2: -19 ≤ RSRQ < -18.5 dB
	 * * ...
	 * * 32: -4 ≤ RSRQ < -3.5 dB
	 * * 33: -3.5 ≤ RSRQ < -3 dB
	 * * 34: -3 ≤ RSRQ dB
	 * * 35: -3 ≤ RSRQ < -2.5 dB
	 * * 36: -2.5 ≤ RSRQ < -2 dB
	 * * ...
	 * * 45: 2 ≤ RSRQ < 2.5 dB
	 * * 46: 2.5 ≤ RSRQ dB
	 * * @ref LTE_LC_CELL_RSRQ_INVALID : not known or not detectable.
	 */
	int16_t rsrq;

	/**
	 * Estimated TX repetitions.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_IDLE, this value is the initial preamble
	 * repetition level in (N)PRACH based on CE level and network configuration.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_CONNECTED, this value is the latest physical data
	 * channel (N)PUSCH transmission repetition level.
	 *
	 * See 3GPP TS 36.331 and 36.213 for details.
	 */
	int16_t tx_rep;

	/**
	 * Estimated RX repetitions.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_IDLE, this value is the initial random access
	 * control channel (M/NPDCCH) repetition level based on CE level and network configuration.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_CONNECTED, this value is the latest physical data
	 * channel (N)PDSCH reception repetition level.
	 *
	 * See 3GPP TS 36.331 and 36.213 for details.
	 */
	int16_t rx_rep;

	/** Physical cell ID of evaluated cell. */
	int16_t phy_cid;

	/**
	 * Current band information.
	 *
	 * 0 when band information is not available.
	 *
	 * See 3GPP TS 36.101 for details.
	 */
	int16_t band;

	/**
	 * Current signal-to-noise ratio at time of report.
	 *
	 * * 0:   SNR < -24 dB
	 * * 1:  -24 dB <= SNR < -23 dB
	 * * 2:  -23 dB <= SNR < -22 dB
	 * * ...
	 * * 47:  22 dB <= SNR < 23 dB
	 * * 48:  23 dB <= SNR < 24 dB
	 * * 49:  24 dB <= SNR
	 * * 127: Not known or not detectable.
	 */
	int16_t snr;

	/**
	 * Estimated TX power in dBm.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_IDLE, the value is for first preamble
	 * transmission in (N)PRACH.
	 *
	 * If @c rrc_state is @ref LTE_LC_RRC_MODE_CONNECTED, the value is the latest physical data
	 * channel (N)PUSCH transmission power level.
	 *
	 * See 3GPP TS 36.101 and 36.213 for details.
	 */
	int16_t tx_power;

	/** Mobile Country Code. */
	int mcc;

	/** Mobile Network Code. */
	int mnc;

	/** E-UTRAN cell ID. */
	uint32_t cell_id;
};

/**
 * Specifies which type of search the modem should perform when a neighbor cell measurement is
 * started.
 *
 * When using search types up to @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE, the result
 * contains parameters from current/serving cell and optionally up to
 * @kconfig{CONFIG_LTE_NEIGHBOR_CELLS_MAX} neighbor cells.
 *
 * Result notification for Global Cell ID (GCI) search types include Cell ID, PLMN and TAC for up to
 * @c gci_count (@ref lte_lc_ncellmeas_params) surrounding cells.
 */
enum lte_lc_neighbor_search_type {
	/**
	 * The modem searches the network it is registered to (RPLMN) based on previous cell
	 * history.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT = 1,

	/**
	 * The modem starts with the same search method as the default type. If needed, it
	 * continues to search by measuring the radio conditions and makes assumptions on where
	 * networks might be deployed, in other words, a light search. The search is limited to
	 * bands that are valid for the area of the current ITU-T region. If RPLMN is not found
	 * based on previous cell history, the modem accepts any found PLMN.
	 *
	 * @note This search type is only supported by modem firmware versions >= 1.3.1.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT = 2,

	/**
	 * The modem follows the same procedure as for
	 * @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT, but will continue to perform a complete
	 * search instead of a light search, and the search is performed for all supported bands.
	 *
	 * @note This search type is only supported by modem firmware versions >= 1.3.1.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE = 3,

	/**
	 * GCI search, option 1. Modem searches EARFCNs based on previous cell history.
	 *
	 * @note This search type is only supported by modem firmware versions >= 1.3.4.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT = 4,

	/**
	 * GCI search, option 2. Modem starts with the same search method as in
	 * @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT. If less than gci_count cells were found,
	 * the modem performs a light search on bands that are valid for the area of the current
	 * ITU-T region.
	 *
	 * @note This search type is only supported by modem firmware versions >= 1.3.4.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT = 5,

	/**
	 * GCI search, option 3. Modem starts with the same search method as in
	 * @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT. If less than gci_count cells were found,
	 * the modem performs a complete search on all supported bands.
	 *
	 * @note This search type is only supported by modem firmware versions >= 1.3.4.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE = 6,
};

/** Neighbor cell measurement initiation parameters. */
struct lte_lc_ncellmeas_params {
	/** Search type, @ref lte_lc_neighbor_search_type. */
	enum lte_lc_neighbor_search_type search_type;

	/**
	 * Maximum number of GCI cells to be searched. Integer, range: 2-15.
	 *
	 * Current cell is counted as one cell. Mandatory with the GCI search types, ignored with
	 * other search types.
	 *
	 * @note GCI search types are only supported by modem firmware versions >= 1.3.4.
	 */
	uint8_t gci_count;
};


/** Search pattern type. */
enum lte_lc_periodic_search_pattern_type {
	/** Range search pattern. */
	LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE = 0,

	/** Table search pattern. */
	LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE = 1,
};

/** Configuration for periodic search of type @ref LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE. */
struct lte_lc_periodic_search_range_cfg {
	/**
	 * Sleep time between searches in the beginning of the range.
	 *
	 * Allowed values: 0 - 65535 seconds.
	 */
	uint16_t initial_sleep;

	/**
	 * Sleep time between searches in the end of the range.
	 *
	 * Allowed values: 0 - 65535 seconds.
	 */
	uint16_t final_sleep;

	/**
	 * Optional target time in minutes for achieving the @c final_sleep value.
	 *
	 * This can be used to determine angle factor between the initial and final sleep times.
	 * The timeline for the @c time_to_final_sleep starts from the beginning of the search
	 * pattern. If given, the value cannot be greater than the value of the
	 * @c pattern_end_point  value in the same search pattern. If not given, the angle factor
	 * is calculated by using the @c pattern_end_point value so, that the @c final_sleep
	 * value is reached at the point of @c pattern_end_point.
	 *
	 * Allowed values:
	 * * -1: Not used
	 * * 0 - 1080 minutes
	 */
	int16_t time_to_final_sleep;

	/**
	 * Time that must elapse before entering the next search pattern.
	 *
	 * The timeline for @c pattern_end_point starts from the beginning of the limited service
	 * starting point, which is the moment when the first sleep period started.
	 *
	 * Allowed values:
	 * * 0 - 1080 minutes.
	 */
	int16_t pattern_end_point;
};

/**
 * Configuration for periodic search of type @ref LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE.
 *
 * 1 to 5 sleep time values for sleep between searches can be configured. It is mandatory to provide
 * @c val_1, while the rest are optional. Unused values must be set to -1. After going through all
 * values, the last value of the last search pattern is repeated, if not configured differently by
 * the @c loop or @c return_to_pattern parameters.
 *
 * Allowed values:
 * * -1: Value unused.
 * * 0 - 65535 seconds.
 */
struct lte_lc_periodic_search_table_cfg {
	/** Mandatory sleep time. */
	int val_1;

	/**
	 * Optional sleep time.
	 *
	 * Must be set to -1 if not used.
	 */
	int val_2;

	/**
	 * Optional sleep time.
	 *
	 * @c val_2 must be configured for this parameter to take effect.
	 *
	 *  Must be set to -1 if not used.
	 */
	int val_3;

	/**
	 * Optional sleep time.
	 *
	 * @c val_3 must be configured for this parameter to take effect.
	 *
	 * Must be set to -1 if not used.
	 */
	int val_4;

	/**
	 * Optional sleep time.
	 *
	 * @c val_4 must be configured for this parameter to take effect.
	 *
	 * Must be set to -1 if not used.
	 */
	int val_5;
};

/**
 * Periodic search pattern.
 *
 * A search pattern may be of either 'range' or 'table' type.
 */
struct lte_lc_periodic_search_pattern {
	/** Search pattern type. */
	enum lte_lc_periodic_search_pattern_type type;

	union {
		/** Configuration for periodic search of type 'range'. */
		struct lte_lc_periodic_search_range_cfg range;

		/** Configuration for periodic search of type 'table'. */
		struct lte_lc_periodic_search_table_cfg table;
	};
};

/** Periodic search configuration. */
struct lte_lc_periodic_search_cfg {
	/**
	 * Indicates if the last given pattern is looped from the beginning when the pattern has
	 * ended.
	 *
	 * If several patterns are configured, this impacts only the last pattern.
	 */
	bool loop;

	/**
	 * Indicates if the modem can return to a given search pattern with shorter sleeps, for
	 * example, when radio conditions change and the given pattern index has already been
	 * exceeded.
	 *
	 * Allowed values:
	 * * 0: No return pattern.
	 * * 1 - 4: Return to search pattern index 1..4.
	 */
	uint16_t return_to_pattern;

	/**
	 * Indicates if band optimization shall be used.
	 *
	 * * 0: No optimization. Every periodic search shall be all band search.
	 * * 1: Use default optimizations predefined by modem. Predefinition depends on
	 *      the active data profile.
	 *      See "nRF91 AT Commands - Command Reference Guide" for more information.
	 * * 2 - 20: Every n periodic search must be an all band search.
	 */
	uint16_t band_optimization;

	/** The number of valid patterns. Range 1 - 4. */
	size_t pattern_count;

	/** Array of periodic search patterns. */
	struct lte_lc_periodic_search_pattern patterns[4];
};

/** Callback for modem functional mode changes. */
struct lte_lc_cfun_cb {
	void (*callback)(enum lte_lc_func_mode, void *ctx);
	void *context;
};

/**
 * Define a callback for functional mode changes through lte_lc_func_mode_set().
 *
 * @param name Callback name.
 * @param _callback Callback function.
 * @param _context User-defined context.
 */
#define LTE_LC_ON_CFUN(name, _callback, _context)                                                  \
	static void _callback(enum lte_lc_func_mode, void *ctx);                                   \
	STRUCT_SECTION_ITERABLE(lte_lc_cfun_cb, lte_lc_cfun_cb_##name) = {                         \
		.callback = _callback,                                                             \
		.context = _context,                                                               \
	};

/** LTE event. */
struct lte_lc_evt {
	/** Event type. */
	enum lte_lc_evt_type type;

	union {
		/** Payload for event @ref LTE_LC_EVT_NW_REG_STATUS. */
		enum lte_lc_nw_reg_status nw_reg_status;

		/** Payload for event @ref LTE_LC_EVT_RRC_UPDATE. */
		enum lte_lc_rrc_mode rrc_mode;

		/** Payload for event @ref LTE_LC_EVT_PSM_UPDATE. */
		struct lte_lc_psm_cfg psm_cfg;

		/** Payload for event @ref LTE_LC_EVT_EDRX_UPDATE. */
		struct lte_lc_edrx_cfg edrx_cfg;

		/** Payload for event @ref LTE_LC_EVT_CELL_UPDATE. */
		struct lte_lc_cell cell;

		/** Payload for event @ref LTE_LC_EVT_LTE_MODE_UPDATE. */
		enum lte_lc_lte_mode lte_mode;

		/**
		 * Payload for events @ref LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING,
		 * @ref LTE_LC_EVT_MODEM_SLEEP_EXIT and @ref LTE_LC_EVT_MODEM_SLEEP_ENTER.
		 */
		struct lte_lc_modem_sleep modem_sleep;

		/** Payload for event @ref LTE_LC_EVT_MODEM_EVENT. */
		enum lte_lc_modem_evt modem_evt;

		/**
		 * Payload for event @ref LTE_LC_EVT_TAU_PRE_WARNING.
		 *
		 * Time until next Tracking Area Update in milliseconds.
		 */
		uint64_t time;

		/** Payload for event @ref LTE_LC_EVT_NEIGHBOR_CELL_MEAS. */
		struct lte_lc_cells_info cells_info;
	};
};

/**
 * Handler for LTE events.
 *
 * @param[in] evt Event.
 */
typedef void(*lte_lc_evt_handler_t)(const struct lte_lc_evt *const evt);

/**
 * Register handler for LTE events.
 *
 * @param[in] handler Event handler.
 */
void lte_lc_register_handler(lte_lc_evt_handler_t handler);

/**
 * De-register handler for LTE events.
 *
 * @param[in] handler Event handler.
 *
 * @retval 0 if successful.
 * @retval -ENXIO if the handler was not found.
 * @retval -EINVAL if the handler was a @c NULL pointer.
 */
int lte_lc_deregister_handler(lte_lc_evt_handler_t handler);

/**
 * Initialize the library and configure the modem.
 *
 * @note A follow-up call to lte_lc_connect() or lte_lc_connect_async() must be made to establish
 *       an LTE connection. The library can be initialized only once, and subsequent calls will
 *       return @c 0.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_init(void);

/**
 * Connect to LTE network.
 *
 * @note Before calling this function, a call to lte_lc_init() must be made, otherwise @c -EPERM is
 *       returned.
 *
 * @note After initialization, the system mode will be set to the default mode selected with Kconfig
 *       and LTE preference set to automatic selection.
 *
 * @retval 0 if successful.
 * @retval -EPERM if the library was not initialized.
 * @retval -EFAULT if an AT command failed.
 * @retval -ETIMEDOUT if a connection attempt timed out before the device was
 *	   registered to a network.
 * @retval -EINPROGRESS if a connection establishment attempt is already in progress.
 */
int lte_lc_connect(void);

/**
 * Initialize the library, configure the modem and connect to LTE network.
 *
 * The function blocks until connection is established, or the connection attempt times out.
 *
 * @note The library can be initialized only once, and repeated calls will return @c 0.
 *       lte_lc_connect_async() should be used on subsequent calls.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 * @retval -ETIMEDOUT if a connection attempt timed out before the device was
 *	   registered to a network.
 * @retval -EINPROGRESS if a connection establishment attempt is already in progress.
 */
int lte_lc_init_and_connect(void);

/**
 * Connect to LTE network.
 *
 * The function returns immediately.
 *
 * @note The library must be initialized before this function is called.
 *
 * @param[in] handler Event handler for receiving LTE events. The parameter can be @c NULL if an
 *                    event handler is already registered.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if no event handler was registered.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_connect_async(lte_lc_evt_handler_t handler);

/**
 * Initialize the library, configure the modem and connect to LTE network.
 *
 * The function returns immediately.
 *
 * @note The library can be initialized only once, and repeated calls will return @c 0.
 *       lte_lc_connect() should be used on subsequent calls.
 *
 * @param[in] handler Event handler for receiving LTE events. The parameter can be @c NULL if an
 *                    event handler is already registered.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 * @retval -EINVAL if no event handler was registered.
 */
int lte_lc_init_and_connect_async(lte_lc_evt_handler_t handler);

/**
 * Deinitialize the library and power off the modem.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_deinit(void);

/**
 * Set the modem to offline mode.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_offline(void);

/**
 * Set the modem to power off mode.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_power_off(void);

/**
 * Set the modem to normal mode.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_normal(void);

/**
 * Set modem PSM parameters.
 *
 * Requested periodic TAU (RPTAU) and requested active time (RAT) are used when PSM mode is
 * subsequently enabled using lte_lc_psm_req().
 *
 * For reference see 3GPP 27.007 Ch. 7.38.
 *
 * @param[in] rptau Requested periodic TAU as a null-terminated string.
 *                  Set to @c NULL to use manufacturer-specific default value.
 * @param[in] rat Requested active time as a null-terminated string.
 *                Set to @c NULL to use manufacturer-specific default value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_psm_param_set(const char *rptau, const char *rat);

/**
 * Request modem to enable or disable power saving mode (PSM).
 *
 * PSM parameters can be set using @kconfig{CONFIG_LTE_PSM_REQ_RPTAU} and
 * @kconfig{CONFIG_LTE_PSM_REQ_RAT}, or by calling lte_lc_psm_param_set().
 *
 * @note @kconfig{CONFIG_LTE_PSM_REQ} can be set to enable PSM, which is generally sufficient. This
 *       option allows explicit disabling/enabling of PSM requesting after modem initialization.
 *       Calling this function for run-time control is possible, but it should be noted that
 *       conflicts may arise with the value set by @kconfig{CONFIG_LTE_PSM_REQ} if it is called
 *       during modem initialization.
 *
 * @param[in] enable @c true to enable PSM, @c false to disable PSM.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_psm_req(bool enable);

/**
 * Get the current PSM (Power Saving Mode) configuration.
 *
 * @param[out] tau Periodic TAU interval in seconds. Positive integer,
 *                 or @c -1 if timer is deactivated.
 * @param[out] active_time Active time in seconds. Positive integer,
 *                         or @c -1 if timer is deactivated.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EBADMSG if no active time and/or TAU value was received, including the case when
 *         modem is not registered to network.
 */
int lte_lc_psm_get(int *tau, int *active_time);

/**
 * Set the Paging Time Window (PTW) value to be used with eDRX.
 *
 * eDRX can be requested using lte_lc_edrx_req(). PTW is set individually for LTE-M and NB-IoT.
 *
 * Requesting a specific PTW configuration should be done with caution. The requested value must be
 * compliant with the eDRX value that is configured, and it is usually best to let the modem
 * use default PTW values.
 *
 * For reference to which values can be set, see subclause 10.5.5.32 of 3GPP TS 24.008.
 *
 * @param[in] mode LTE mode to which the PTW value applies.
 * @param[in] ptw Paging Time Window value as a null-terminated string.
 *                Set to @c NULL to use manufacturer-specific default value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_ptw_set(enum lte_lc_lte_mode mode, const char *ptw);

/**
 * Set the requested eDRX value.
 *
 * eDRX can be requested using lte_lc_edrx_req(). eDRX value is set individually for LTE-M and
 * NB-IoT.
 *
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @param[in] mode LTE mode to which the eDRX value applies.
 * @param[in] edrx eDRX value as a null-terminated string.
 *                 Set to @c NULL to use manufacturer-specific default.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx);

/**
 * Request modem to enable or disable use of eDRX.
 *
 * eDRX parameters can be set using @kconfig{CONFIG_LTE_EDRX_REQ_VALUE_LTE_M},
 * @kconfig{CONFIG_LTE_EDRX_REQ_VALUE_NBIOT}, @kconfig{CONFIG_LTE_PTW_VALUE_LTE_M} and
 * @kconfig{CONFIG_LTE_PTW_VALUE_NBIOT}, or by calling lte_lc_edrx_param_set() and
 * lte_lc_ptw_set().
 *
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @note @kconfig{CONFIG_LTE_EDRX_REQ} can be set to enable eDRX, which is generally sufficient.
 *       This option allows explicit disabling/enabling of eDRX requesting after modem
 *       initialization. Calling this function for run-time control is possible, but it should be
 *       noted that conflicts may arise with the value set by @kconfig{CONFIG_LTE_EDRX_REQ} if it is
 *       called during modem initialization.
 *
 * @param[in] enable @c true to enable eDRX, @c false to disable eDRX.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_edrx_req(bool enable);

/**
 * Get the eDRX parameters currently provided by the network.
 *
 * @param[out] edrx_cfg eDRX configuration.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EBADMSG if parsing of the AT command response failed.
 */
int lte_lc_edrx_get(struct lte_lc_edrx_cfg *edrx_cfg);

/**
 * Set the RAI value to be used.
 *
 * RAI can be subsequently enabled using lte_lc_rai_req().
 *
 * For reference see 3GPP 24.301 Ch. 9.9.4.25.
 *
 * @deprecated Use @kconfig{CONFIG_LTE_RAI_REQ} and socket options SO_RAI_* instead.
 *
 * @note This feature is only supported by modem firmware versions < 2.0.0.
 *
 * @param[in] value RAI value as a null-terminated string.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
__deprecated int lte_lc_rai_param_set(const char *value);

/**
 * Request modem to enable or disable use of RAI.
 *
 * Used RAI value can be set using @kconfig{CONFIG_LTE_RAI_REQ_VALUE} or by calling
 * lte_lc_rai_param_set().
 *
 * @deprecated Use @kconfig{CONFIG_LTE_RAI_REQ} and socket options SO_RAI_* instead.
 *
 * @note This feature is only supported by modem firmware versions < 2.0.0.
 *
 * @param[in] enable @c true to enable RAI, @c false to disable RAI.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 * @retval -EOPNOTSUPP if RAI is not supported in the current system mode.
 */
__deprecated int lte_lc_rai_req(bool enable);

/**
 * Get the current network registration status.
 *
 * @param[out] status Network registration status.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the network registration could not be retrieved from the modem.
 */
int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status);

/**
 * Set the modem's system mode and LTE preference.
 *
 * @param[in] mode System mode.
 * @param[in] preference System mode preference.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the network registration could not be retrieved from the modem.
 */
int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference);

/**
 * Get the modem's system mode and LTE preference.
 *
 * @param[out] mode System mode.
 * @param[out] preference System mode preference. Can be @c NULL.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the system mode could not be retrieved from the modem.
 */
int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference);

/**
 * Set the modem's functional mode.
 *
 * @param[in] mode Functional mode.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the functional mode could not be retrieved from the modem.
 */
int lte_lc_func_mode_set(enum lte_lc_func_mode mode);

/**
 * Get the modem's functional mode.
 *
 * @param[out] mode Functional mode.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the functional mode could not be retrieved from the modem.
 */
int lte_lc_func_mode_get(enum lte_lc_func_mode *mode);

/**
 * Get the currently active LTE mode.
 *
 * @param[out] mode LTE mode.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the current LTE mode could not be retrieved.
 * @retval -EBADMSG if the LTE mode was not recognized.
 */
int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode);

/**
 * Initiate a neighbor cell measurement.
 *
 * The result of the measurement is reported back as an event of the type
 * @ref LTE_LC_EVT_NEIGHBOR_CELL_MEAS, meaning that an event handler must be registered to receive
 * the information. Depending on the network conditions, LTE connection state and requested search
 * type, it may take a while before the measurement result is ready and reported back. After the
 * event is received, the neighbor cell measurements are automatically stopped.
 *
 * @note This feature is only supported by modem firmware versions >= 1.3.0.
 *
 * @param[in] params Search type parameters or @c NULL to initiate a measurement with the default
 *                   parameters. See @ref lte_lc_ncellmeas_params for more information.
 *
 * @retval 0 if neighbor cell measurement was successfully initiated.
 * @retval -EFAULT if AT command failed.
 * @retval -EINPROGRESS if a neighbor cell measurement is already in progress.
 */
int lte_lc_neighbor_cell_measurement(struct lte_lc_ncellmeas_params *params);

/**
 * Cancel an ongoing neighbor cell measurement.
 *
 * @retval 0 if neighbor cell measurement was cancelled.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_neighbor_cell_measurement_cancel(void);

/**
 * Get connection evaluation parameters.
 *
 * Connection evaluation parameters can be used to determine the energy efficiency of data
 * transmission before the actual data transmission.
 *
 * @param[out] params Connection evaluation parameters.
 *
 * @return Zero on success, negative errno code if the API call fails, and a positive error
 *         code if the API call succeeds but connection evaluation fails due to modem/network
 *         related reasons.
 *
 * @retval 0 if evaluation succeeded.
 * @retval 1 if evaluation failed, no cell available.
 * @retval 2 if evaluation failed, UICC not available.
 * @retval 3 if evaluation failed, only barred cells available.
 * @retval 4 if evaluation failed, radio busy (e.g GNSS activity).
 * @retval 5 if evaluation failed, aborted due to higher priority operation.
 * @retval 6 if evaluation failed, UE not registered to network.
 * @retval 7 if evaluation failed, unspecified.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EOPNOTSUPP if connection evaluation is not available in the current functional mode.
 * @retval -EBADMSG if parsing of the AT command response failed.
 */
int lte_lc_conn_eval_params_get(struct lte_lc_conn_eval_params *params);

/**
 * Enable modem domain events.
 *
 * See @ref lte_lc_modem_evt for more information on which events may be received.
 * An event handler must be registered to receive events.
 *
 * @note This feature is only supported by modem firmware versions >= 1.3.0.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_modem_events_enable(void);

/**
 * Disable modem domain events.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_modem_events_disable(void);

/**
 * Configure periodic searches.
 *
 * This configuration affects the periodic searches that the modem performs in limited service state
 * to obtain normal service. See @ref lte_lc_periodic_search_cfg and
 * "nRF91 AT Commands - Command Reference Guide" for more information and in-depth explanations of
 * periodic search configuration.
 *
 * @param[in] cfg Periodic search configuration.
 *
 * @retval 0 if the configuration was successfully sent to the modem.
 * @retval -EINVAL if an input parameter was @c NULL or contained an invalid pattern count.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 * @retval -EBADMSG if the modem responded with an error to an AT command.
 */
int lte_lc_periodic_search_set(const struct lte_lc_periodic_search_cfg *const cfg);

/**
 * Get the configured periodic search parameters.
 *
 * @param[out] cfg Periodic search configuration.
 *
 * @retval 0 if a configuration was found and populated to the provided pointer.
 * @retval -EINVAL if input parameter was @c NULL.
 * @retval -ENOENT if no periodic search was not configured.
 * @retval -EFAULT if an AT command failed.
 * @retval -EBADMSG if the modem responded with an error to an AT command or the
 *         response could not be parsed.
 */
int lte_lc_periodic_search_get(struct lte_lc_periodic_search_cfg *const cfg);

/**
 * Clear the configured periodic search parameters.
 *
 * @retval 0 if the configuration was cleared.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 * @retval -EBADMSG if the modem responded with an error to an AT command.
 */
int lte_lc_periodic_search_clear(void);

/**
 * Request an extra search.
 *
 * This can be used for example when modem is in sleep state between periodic searches. The search
 * is performed only when the modem is in sleep state between periodic searches.
 *
 * @retval 0 if the search request was successfully delivered to the modem.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 */
int lte_lc_periodic_search_request(void);

/**
 * Read the current reduced mobility mode.
 *
 * @note This feature is only supported by modem firmware versions >= 1.3.2.
 *
 * @param[out] mode Reduced mobility mode.
 *
 * @retval 0 if a mode was found and written to the provided pointer.
 * @retval -EINVAL if input parameter was @c NULL.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_reduced_mobility_get(enum lte_lc_reduced_mobility_mode *mode);

/**
 * Set reduced mobility mode.
 *
 * @note This feature is only supported by modem firmware versions >= 1.3.2.
 *
 * @param[in] mode Reduced mobility mode.
 *
 * @retval 0 if the new reduced mobility mode was accepted by the modem.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_reduced_mobility_set(enum lte_lc_reduced_mobility_mode mode);

/**
 * Reset modem to factory settings.
 *
 * This operation is allowed only when the modem is not activated.
 *
 * @note This feature is only supported by modem firmware versions >= 1.3.0.
 *
 * @param[in] type Factory reset type.
 *
 * @retval 0 if factory reset was performed successfully.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_factory_reset(enum lte_lc_factory_reset_type type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* LTE_LC_H__ */
