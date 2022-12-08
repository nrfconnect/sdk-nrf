/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_
#define ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file lte_lc.h
 *
 * @defgroup lte_lc LTE Link Controller
 *
 * @{
 *
 * @brief Public APIs for the LTE Link Controller.
 */

/* NOTE: enum lte_lc_nw_reg_status maps directly to the registration status
 *	 as returned by the AT command "AT+CEREG?".
 */
enum lte_lc_nw_reg_status {
	LTE_LC_NW_REG_NOT_REGISTERED		= 0,
	LTE_LC_NW_REG_REGISTERED_HOME		= 1,
	LTE_LC_NW_REG_SEARCHING			= 2,
	LTE_LC_NW_REG_REGISTRATION_DENIED	= 3,
	LTE_LC_NW_REG_UNKNOWN			= 4,
	LTE_LC_NW_REG_REGISTERED_ROAMING	= 5,
	LTE_LC_NW_REG_REGISTERED_EMERGENCY	= 8,
	LTE_LC_NW_REG_UICC_FAIL			= 90
};

enum lte_lc_system_mode {
	LTE_LC_SYSTEM_MODE_NONE,
	LTE_LC_SYSTEM_MODE_LTEM,
	LTE_LC_SYSTEM_MODE_NBIOT,
	LTE_LC_SYSTEM_MODE_GPS,
	LTE_LC_SYSTEM_MODE_LTEM_GPS,
	LTE_LC_SYSTEM_MODE_NBIOT_GPS,
	LTE_LC_SYSTEM_MODE_LTEM_NBIOT,
	LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS,
};

/** LTE mode. The values for LTE-M and NB-IoT correspond to the values for the
 *  AcT field in an AT+CEREG response.
 */
enum lte_lc_lte_mode {
	LTE_LC_LTE_MODE_NONE	= 0,
	LTE_LC_LTE_MODE_LTEM	= 7,
	LTE_LC_LTE_MODE_NBIOT	= 9,
};

/** LTE mode preference. If more than one LTE system mode is enabled, the modem
 *  can select the mode that best meets the criteria set by this configuration.
 *  The LTE mode preference does not affect the way GPS operates.
 *
 *  Note that there's a distinction between preferred and prioritized mode.
 */
enum lte_lc_system_mode_preference {
	/** No LTE preference, automatically selected by the modem. */
	LTE_LC_SYSTEM_MODE_PREFER_AUTO = 0,

	/** LTE-M is preferred over PLMN selection. The modem will prioritize to
	 *  use LTE-M and switch over to a PLMN where LTE-M is available whenever
	 *  possible.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_LTEM,

	/** NB-IoT is preferred over PLMN selection. The modem will prioritize to
	 *  use NB-IoT and switch over to a PLMN where NB-IoT is available
	 *  whenever possible.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_NBIOT,

	/** LTE-M is preferred, but PLMN selection is more important. The modem
	 *  will prioritize to stay on home network and switch over to NB-IoT
	 *  if LTE-M is not available.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO,

	/** NB-IoT is preferred, but PLMN selection is more important. The modem
	 *  will prioritize to stay on home network and switch over to LTE-M
	 *  if NB-IoT is not available.
	 */
	LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO
};

/** @brief Functional modes, used to control RF functionality in the modem.
 *
 *  @note The functional modes map directly to the functional modes as described
 *	  in "nRF91 AT Commands - Command Reference Guide". Please refer to the
 *	  AT command guide to verify if a functional mode is supported by a
 *	  given modem firmware version.
 */
enum lte_lc_func_mode {
	/* Sets the device to minimum functionality. Disables both transmit and
	 * receive RF circuits and deactivates LTE and GNSS.
	 */
	LTE_LC_FUNC_MODE_POWER_OFF		= 0,

	/* Sets the device to full functionality. Both LTE and GNSS will become
	 * active if the respective system modes are enabled.
	 */
	LTE_LC_FUNC_MODE_NORMAL			= 1,

	/* Sets the device to receive only functionality. Can be used, for example,
	 * to preevaluate connections with @ref lte_lc_conn_eval_params_get.
	 * Available for modem firmware versions >= 1.3.0.
	 */
	LTE_LC_FUNC_MODE_RX_ONLY		= 2,

	/* Sets the device to flight mode. Disables both transmit and receive RF
	 * circuits and deactivates LTE and GNSS services.
	 */
	LTE_LC_FUNC_MODE_OFFLINE		= 4,

	/* Deactivates LTE without shutting down GNSS services. */
	LTE_LC_FUNC_MODE_DEACTIVATE_LTE		= 20,

	/* Activates LTE without changing GNSS. */
	LTE_LC_FUNC_MODE_ACTIVATE_LTE		= 21,

	/* Deactivates GNSS without shutting down LTE services. */
	LTE_LC_FUNC_MODE_DEACTIVATE_GNSS	= 30,

	/* Activates GNSS without changing LTE. */
	LTE_LC_FUNC_MODE_ACTIVATE_GNSS		= 31,

	/* Deactivates UICC. */
	LTE_LC_FUNC_MODE_DEACTIVATE_UICC	= 40,

	/* Activates UICC. */
	LTE_LC_FUNC_MODE_ACTIVATE_UICC		= 41,

	/* Sets the device to flight mode without shutting down UICC. */
	LTE_LC_FUNC_MODE_OFFLINE_UICC_ON	= 44,
};

enum lte_lc_evt_type {
	/** @brief Event received carrying information about the modems network registration
	 *         status.
	 *         The associated payload is the nw_reg_status member of
	 *	   type @ref lte_lc_nw_reg_status in the event.
	 */
	LTE_LC_EVT_NW_REG_STATUS,

	/** @brief Event received carrying information about PSM updates. Contains PSM parameters
	 *	   given by the network.
	 *         The associated payload is the psm_cfg member of type @ref lte_lc_psm_cfg
	 *	   in the event.
	 */
	LTE_LC_EVT_PSM_UPDATE,

	/** @brief Event received carrying information about eDRX updates. Contains eDRX parameters
	 *	   given by the network.
	 *         The associated payload is the edrx_cfg member of type @ref lte_lc_edrx_cfg
	 *	   in the event.
	 */
	LTE_LC_EVT_EDRX_UPDATE,

	/** @brief Event received carrying information about the modems RRC state.
	 *         The associated payload is the rrc_mode member of type @ref lte_lc_rrc_mode
	 *	   in the event.
	 */
	LTE_LC_EVT_RRC_UPDATE,

	/** @brief Event received carrying information about the currently connected cell.
	 *         The associated payload is the cell member of type @ref lte_lc_cell
	 *	   in the event. Note that only the cell.tac and cell.id members are populated
	 *	   in the event payload. The rest are expected to be zero.
	 */
	LTE_LC_EVT_CELL_UPDATE,

	/** @brief The currently active LTE mode is updated. If a system mode that
	 *	   enables both LTE-M and NB-IoT is configured, the modem may change
	 *	   the currently active LTE mode based on the system mode preference
	 *	   and network availability. This event will then indicate which
	 *	   LTE mode is currently used by the modem.
	 *         The associated payload is the lte_mode member of type @ref lte_lc_lte_mode
	 *	   in the event.
	 */
	LTE_LC_EVT_LTE_MODE_UPDATE,

	/** @brief Tracking Area Update pre-warning.
	 *	   This event will be received a configurable amount of time before TAU is
	 *	   scheduled to occur. This gives the application the opportunity to send
	 *	   data over the network before the TAU happens, thus saving power by
	 *	   avoiding sending data and the TAU separately.
	 *         The associated payload is the time member of type uint64_t in the event.
	 */
	LTE_LC_EVT_TAU_PRE_WARNING,

	/** @brief Event containing results from neighbor cell measurements.
	 *         The associated payload is the cells_info member of type @ref lte_lc_cells_info
	 *	   in the event.
	 */
	LTE_LC_EVT_NEIGHBOR_CELL_MEAS,

	/** @brief Modem sleep pre-warning
	 *	   This event will be received a configurable amount of time before
	 *	   the modem exits sleep. The time parameter associated with this
	 *	   event signifies the time until modem exits sleep.
	 *         The associated payload is the modem_sleep member of type @ref lte_lc_modem_sleep
	 *	   in the event.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING,

	/** @brief This event will be received when the modem exits sleep.
	 *         The associated payload is the modem_sleep member of type @ref lte_lc_modem_sleep
	 *	   in the event.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT,

	/** @brief This event will be received when the modem enters sleep.
	 *         The time parameter associated with this event signifies
	 *         the duration of the sleep.
	 *         The associated payload is the modem_sleep member of type @ref lte_lc_modem_sleep
	 *	   in the event.
	 */
	LTE_LC_EVT_MODEM_SLEEP_ENTER,

	/** @brief Modem domain event carrying information about modem operation.
	 *         The associated payload is the modem_evt member of type @ref lte_lc_modem_evt
	 *	   in the event.
	 */
	LTE_LC_EVT_MODEM_EVENT,
};

enum lte_lc_rrc_mode {
	LTE_LC_RRC_MODE_IDLE		= 0,
	LTE_LC_RRC_MODE_CONNECTED	= 1,
};

struct lte_lc_psm_cfg {
	int tau;		/* Periodic Tracking Area Update interval */
	int active_time;	/* Active-time (time from RRC idle to PSM) */
};

struct lte_lc_edrx_cfg {
	/* LTE mode for which the configuration is valid. */
	enum lte_lc_lte_mode mode;
	/* eDRX interval value [s] */
	float edrx;
	/* Paging time window [s] */
	float ptw;
};

#define LTE_LC_CELL_TIMING_ADVANCE_MAX		20512
#define LTE_LC_CELL_TIMING_ADVANCE_INVALID	65535
#define LTE_LC_CELL_EARFCN_MAX			262143
#define LTE_LC_CELL_RSRP_INVALID		255
#define LTE_LC_CELL_RSRQ_INVALID		255
#define LTE_LC_CELL_EUTRAN_ID_INVALID		UINT32_MAX
#define LTE_LC_CELL_EUTRAN_ID_MAX		268435455
#define LTE_LC_CELL_TIME_DIFF_INVALID		0

/** @brief Structure containing neighbor cell information. */
struct lte_lc_ncell {
	/** EARFCN of the neighbor cell, per 3GPP TS 36.101. */
	uint32_t earfcn;

	/** Difference in milliseconds of current cell and neighbor cell
	 *  measurement, in the range -99999 ms < time_diff < 99999 ms.
	 */
	int time_diff;

	/** Physical cell ID. */
	uint16_t phys_cell_id;

	/** RSRP of the neighbor cell. Format is the same as for RSRP member
	 *  of struct @ref lte_lc_cell.
	 */
	int16_t rsrp;

	/** RSRQ of the neighbor cell. Format is the same as for RSRQ member
	 *  of struct @ref lte_lc_cell.
	 */
	int16_t rsrq;
};

/** @brief Structure containing cell information. */
struct lte_lc_cell {
	/** Mobile Country Code. */
	int mcc;

	/** Mobile Network Code. */
	int mnc;

	/** E-UTRAN cell ID, range 0 - LTE_LC_CELL_EUTRAN_ID_MAX */
	uint32_t id;

	/** Tracking area code. */
	uint32_t tac;

	/** EARFCN of the cell, per 3GPP TS 36.101. */
	uint32_t earfcn;

	/** Timing advance decimal value.
	 *  Range [0..20512, TIMING_ADVANCE_NOT_VALID = 65535].
	 *
	 * Note: Timing advance may be reported from past measurements.
	 *	 The parameters @c timing_advance_meas_time and @c measurement_time
	 *	 can be used to evaluate if the parameter is usable.
	 */
	uint16_t timing_advance;

	/** Timing advance measurement time in milliseconds, calculated from modem
	 *  boot time.
	 *  Range 0 - 18 446 744 073 709 551 614 ms.
	 *
	 *  For modem firmware versions >= 1.3.1, timing advance measurement time may
	 *  be reported from the modem. This means that timing advance data
	 *  may now also be available in neighbor cell measurements done in
	 *  RRC idle, even though the timing advance data was captured in RRC connected.
	 *  If the value is not reported by the modem, it is set to 0.
	 */
	uint64_t timing_advance_meas_time;

	/** Measurement time of current cell in milliseconds.
	 *  Range 0 - 18 446 744 073 709 551 614 ms.
	 */
	uint64_t measurement_time;

	/** Physical cell ID. */
	uint16_t phys_cell_id;

	/** RSRP of the neighbor cell.
	 *  -17: RSRP < -156 dBm
	 *  -16: -156 ≤ RSRP < -155 dBm
	 *  ...
	 *  -3: -143 ≤ RSRP < -142 dBm
	 *  -2: -142 ≤ RSRP < -141 dBm
	 *  -1: -141 ≤ RSRP < -140 dBm
	 *  0: RSRP < -140 dBm
	 *  1: -140 ≤ RSRP < -139 dBm
	 *  2: -139 ≤ RSRP < -138 dBm
	 *  ...
	 *  95: -46 ≤ RSRP < -45 dBm
	 *  96: -45 ≤ RSRP < -44 dBm
	 *  97: -44 ≤ RSRP dBm
	 *  255: not known or not detectable
	 */
	int16_t rsrp;

	/** RSRQ of the neighbor cell.
	 *  -30: RSRQ < -34 dB
	 *  -29:	-34 ≤ RSRQ < -33.5 dB
	 *  ...
	 *  -2: -20.5 ≤ RSRQ < -20 dB
	 *  -1:	-20 ≤ RSRQ < -19.5 dB
	 *  0: RSRQ < -19.5 dB
	 *  1: -19.5 ≤ RSRQ < -19 dB
	 *  2: -19 ≤ RSRQ < -18.5 dB
	 *  ...
	 *  32: -4 ≤ RSRQ < -3.5 dB
	 *  33: -3.5 ≤ RSRQ < -3 dB
	 *  34: -3 ≤ RSRQ dB
	 *  35: -3 ≤ RSRQ < -2.5 dB
	 *  36: -2.5 ≤ RSRQ < -2 dB
	 *  ...
	 *  45: 2 ≤ RSRQ < 2.5 dB
	 *  46: 2.5 ≤ RSRQ dB
	 *  255: not known or not detectable.
	 */
	int16_t rsrq;
};


/** @brief Structure containing results of neighbor cell measurements. */
struct lte_lc_cells_info {
	/** The current cell information is valid if the current cell ID is not
	 *  set to LTE_LC_CELL_EUTRAN_ID_INVALID.
	 */
	struct lte_lc_cell current_cell;

	/** The ncells_count member indicates whether or not the neighbor_cells structure
	 *  contains valid neighbor cell information. If it is zero, no neighbor cells were found
	 *  for the current cell.
	 */
	uint8_t ncells_count;

	/** Neighbor cells for the current cell. */
	struct lte_lc_ncell *neighbor_cells;

	/** The gci_cells_count member indicates whether or not the gci_cells structure contains
	 *  valid surrounding cell information from
	 *  GCI search types (@ref lte_lc_neighbor_search_type). If it is zero, no cells were
	 *  found, and the information in the rest of structure members do not contain valid data.
	 */
	uint8_t gci_cells_count;

	/** Surrounding cells found by the GCI search types. */
	struct lte_lc_cell *gci_cells;
};

enum lte_lc_modem_sleep_type {
	LTE_LC_MODEM_SLEEP_PSM			= 1,
	LTE_LC_MODEM_SLEEP_RF_INACTIVITY	= 2,	/* For example eDRX */
	LTE_LC_MODEM_SLEEP_FLIGHT_MODE		= 4,
};

struct lte_lc_modem_sleep {
	enum lte_lc_modem_sleep_type type;

	/* If this value is set to -1. Sleep is considered infinite. */
	int64_t time;
};

enum lte_lc_energy_estimate {
	LTE_LC_ENERGY_CONSUMPTION_EXCESSIVE	= 5,
	LTE_LC_ENERGY_CONSUMPTION_INCREASED	= 6,
	LTE_LC_ENERGY_CONSUMPTION_NORMAL	= 7,
	LTE_LC_ENERGY_CONSUMPTION_REDUCED	= 8,
	LTE_LC_ENERGY_CONSUMPTION_EFFICIENT	= 9,
};

enum lte_lc_tau_triggered {
	/** The evaluated cell is in the Tracking Area Identifier list. When switching to this
	 *  cell, a TAU will not be triggered.
	 */
	LTE_LC_CELL_IN_TAI_LIST		= 0,

	/** The evaluated cell is not in the Tracking Area Identifier list. When switching to this
	 *  cell, a TAU will be triggered.
	 */
	LTE_LC_CELL_NOT_IN_TAI_LIST	= 1,
	LTE_LC_CELL_UNKNOWN		= UINT8_MAX
};

enum lte_lc_ce_level {
	LTE_LC_CE_LEVEL_0_NO_REPETITION		= 0,
	LTE_LC_CE_LEVEL_1_LOW_REPETITION	= 1,
	LTE_LC_CE_LEVEL_2_MEDIUM_REPETITION	= 2,
	LTE_LC_CE_LEVEL_3_LARGE_REPETITION	= 3,
	LTE_LC_CE_LEVEL_UNKNOWN			= UINT8_MAX,
};

/** @brief Reduced mobility mode */
enum lte_lc_reduced_mobility_mode {
	/** Functionality according to the 3GPP relaxed monitoring feature. */
	LTE_LC_REDUCED_MOBILITY_DEFAULT = 0,
	/** Enable Nordic-proprietary reduced mobility feature. */
	LTE_LC_REDUCED_MOBILITY_NORDIC = 1,
	/** Full measurements for best possible mobility. Disable the 3GPP relaxed
	 *  monitoring and Nordic-proprietary reduced mobility features.
	 */
	LTE_LC_REDUCED_MOBILITY_DISABLED = 2,
};

/** @brief Modem domain events. */
enum lte_lc_modem_evt {
	/** Indicates that a light search has been performed. This event gives the
	 *  application the chance to stop using more power when searching networks
	 *  in possibly weaker radio conditions.
	 *  Before sending this event, the modem searches for cells based on previous
	 *  cell history, measures the radio conditions, and makes assumptions on
	 *  where networks might be deployed. This event means that the modem has
	 *  not found a network that it could select based on the 3GPP network
	 *  selection rules from those locations. It does not mean that there are
	 *  no networks to be found in the area. The modem continues more thorough
	 *  searches automatically after sending this status.
	 */
	LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE,

	/** Indicates that a network search has been performed. The modem has found
	 *  a network that it can select according to the 3GPP network selection rules
	 *  or all frequencies have been scanned and a suitable cell was not found.
	 *  In the latter case, the modem enters normal limited service state
	 *  functionality and performs scan for service periodically.
	 */
	LTE_LC_MODEM_EVT_SEARCH_DONE,
	/** The modem has detected a reset loop. A reset loop indicates that
	 *  the modem restricts network attach attempts for the next 30 minutes.
	 *  The timer does not run when the modem has no power or while it stays
	 *  in the reset loop. The modem counts all the resets where the modem
	 *  is not gracefully deinitialized with +CFUN=0 or using an API
	 *  performing the equivalent operation, such as setting the functional
	 *  mode to LTE_LC_FUNC_MODE_POWER_OFF.
	 */
	LTE_LC_MODEM_EVT_RESET_LOOP,

	/** Battery voltage is low and the modem is therefore deactivated. */
	LTE_LC_MODEM_EVT_BATTERY_LOW,

	/** The device is overheated and the modem is therefore deactivated. */
	LTE_LC_MODEM_EVT_OVERHEATED,
};

/** @brief Type of factory reset to perform. */
enum lte_lc_factory_reset_type {
	/** Reset all modem data to factory settings. */
	LTE_LC_FACTORY_RESET_ALL = 0,
	/** Reset user-configurable data to factory settings. */
	LTE_LC_FACTORY_RESET_USER = 1,
};

/** @brief Connection evaluation parameters.
 *
 *  @note For more information on the various connection evaluation parameters, refer to the
 *	  "nRF91 AT Commands - Command Reference Guide".
 */
struct lte_lc_conn_eval_params {
	/** RRC connection state during measurements. */
	enum lte_lc_rrc_mode rrc_state;

	/** Relative estimated energy consumption for data transmission compared to nominal
	 *  consumption.
	 */
	enum lte_lc_energy_estimate energy_estimate;

	/** Value that signifies if the evaluated cell is a part of the Tracking Area Identifier
	 *  list received from the network.
	 */
	enum lte_lc_tau_triggered tau_trig;

	/** Coverage Enhancement level for PRACH depending on RRC state measured in.
	 *
	 *  RRC IDLE: CE level is estimated based on RSRP and network configuration.
	 *
	 *  RRC CONNECTED: Currently configured CE level.
	 *
	 *  CE levels 0-2 are supported in NB-IoT.
	 *  CE levels 0-1 are supported in LTE-M.
	 */
	enum lte_lc_ce_level ce_level;

	/** EARFCN for given cell where EARFCN is per 3GPP TS 36.101. */
	int earfcn;

	/** Reduction in power density in dB. */
	int16_t dl_pathloss;

	/** Current RSRP level at time of report.
	 * -17:  RSRP < -156 dBm
	 * -16: -156 ≤ RSRP < -155 dBm
	 *  ...
	 * -3:  -143 ≤ RSRP < -142 dBm
	 * -2:  -142 ≤ RSRP < -141 dBm
	 * -1:  -141 ≤ RSRP < -140 dBm
	 *  0:   RSRP < -140 dBm
	 *  1:  -140 ≤ RSRP < -139 dBm
	 *  2:  -139 ≤ RSRP < -138 dBm
	 *  ...
	 *  95: -46 ≤ RSRP < -45 dBm
	 *  96: -45 ≤ RSRP < -44 dBm
	 *  97: -44 ≤ RSRP dBm
	 *  255: not known or not detectable
	 */
	int16_t rsrp;

	/** Current RSRQ level at time of report.
	 * -30:  RSRQ < -34 dB
	 * -29: -34 ≤ RSRQ < -33.5 dB
	 * ...
	 * -2:  -20.5 ≤ RSRQ < -20 dB
	 * -1:  -20 ≤ RSRQ < -19.5 dB
	 *  0:   RSRQ < -19.5 dB
	 *  1:  -19.5 ≤ RSRQ < -19 dB
	 *  2:  -19 ≤ RSRQ < -18.5 dB
	 *  ...
	 *  32: -4 ≤ RSRQ < -3.5 dB
	 *  33: -3.5 ≤ RSRQ < -3 dB
	 *  34: -3 ≤ RSRQ dB
	 *  35: -3 ≤ RSRQ < -2.5 dB
	 *  36: -2.5 ≤ RSRQ < -2 dB
	 *  ...
	 *  45:  2 ≤ RSRQ < 2.5 dB
	 *  46:  2.5 ≤ RSRQ dB
	 *  255: not known or not detectable.
	 */
	int16_t rsrq;

	/** Estimate of TX repetitions depending on RRC state measured in. 3GPP TS 36.331 and 36.213
	 *
	 *  RRC IDLE: Initial preamble repetition level in (N)PRACH based on CE level and
	 *	      network configuration.
	 *
	 *  RRC CONNECTED: Latest physical data channel (N)PUSCH transmission repetition level.
	 */
	int16_t tx_rep;

	/** Estimate of RX repetitions depending on RRC state measured in. 3GPP TS 36.331 and 36.213
	 *
	 *  RRC IDLE: Initial Random Access control channel (M/NPDCCH) repetition level based on
	 *	      CE level and network configuration.
	 *
	 *  RRC CONNECTED: Latest physical data channel (N)PDSCH reception repetition level.
	 */
	int16_t rx_rep;

	/** Physical cell ID of evaluated cell. */
	int16_t phy_cid;

	/** Current band information. 0 when band information is not available. 3GPP TS 36.101. */
	int16_t band;

	/** Current signal-to-noise ratio at time of report.
	 *  0:   SNR < -24 dB
	 *  1:  -24 dB <= SNR < -23 dB
	 *  2:  -23 dB <= SNR < -22 dB
	 *  ...
	 *  47:  22 dB <= SNR < 23 dB
	 *  48:  23 dB <= SNR < 24 dB
	 *  49:  24 dB <= SNR
	 *  127: Not known or not detectable.
	 */
	int16_t snr;

	/** Estimate of TX power in dBm depending on RRC state measured in.
	 *  3GPP TS 36.101 and 36.213.
	 *
	 *  RRC IDLE: Estimated TX power level is for first preamble transmission in (N)PRACH).
	 *
	 *  RRC CONNECTED: Latest physical data channel (N)PUSCH transmission power level.
	 */
	int16_t tx_power;

	/** Mobile Country Code. */
	int mcc;

	/** Mobile Network Code. */
	int mnc;

	/** E-UTRAN cell ID. */
	uint32_t cell_id;
};

/** @brief Specifies which type of search the modem should perform when a neighbor
 *	   cell measurement is started.
 *
 *         When using search types up to LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE,
 *         result contains parameters from current/serving cell
 *         and optionally up to CONFIG_LTE_NEIGHBOR_CELLS_MAX neighbor cells.
 *
 *         GCI (Global Cell ID) search types are supported with modem firmware versions >= 1.3.4.
 *         Result notification for GCI search types include Cell ID, PLMN and
 *         TAC for up to gci_count (@ref lte_lc_ncellmeas_params) surrounding cells and
 *         optionally, similarly to search types up to
 *         LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE, a list of neighbor cell measurement
 *         results related to the current_cell (@ref lte_lc_cells_info).
 */
enum lte_lc_neighbor_search_type {
	/** The modem searches the network it is registered to (RPLMN) based on
	 *  previous cell history.
	 *  For modem firmware versions < 1.3.1, this is the only valid option.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT = 1,

	/** The modem starts with the same search method as the default type.
	 *  If needed, it continues to search by measuring the radio conditions
	 *  and makes assumptions on where networks might be deployed, i.e. a light search.
	 *  The search is limited to bands that are valid for the area of the current
	 *  ITU-T region. If RPLMN is not found based on previous cell history, the
	 *  modem accepts any found PLMN.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT = 2,

	/** The modem follows the same procedure as for LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT,
	 *  but will continue to perform a complete search instead of a light search,
	 *  and the search is performed for all supported bands.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE = 3,

	/** GCI search, option 1. Modem searches EARFCNs based on previous cell history. */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT = 4,

	/** GCI search, option 2. Modem starts with the same search method as in
	 *  LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT. If less than gci_count cells were found,
	 *  the modem performs a light search on bands that are valid for the area of
	 *  the current ITU-T region.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT = 5,

	/** GCI search, option 3. Modem starts with the same search method as in
	 *  LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT. If less than gci_count cells were found,
	 *  the modem performs a complete search on all supported bands.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE = 6,
};

/** @brief Neighbor cell measurement initiation parameters.
 *
 *  @note For modem firmware versions < v1.3.1, LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT
 *	  is the only accepted search_type. Other types result in an error.
 *	  For modem firmware versions >= v1.3.4, also GCI search types and gci_count
 *        are accepted.
 */
struct lte_lc_ncellmeas_params {
	/** Search type, @ref lte_lc_neighbor_search_type. */
	enum lte_lc_neighbor_search_type search_type;

	/** Maximum number of cells to be searched. Integer, range: 2-15.
	 *  Mandatory with the GCI search types, ignored with other search types.
	 */
	uint8_t gci_count;
};

/** Configuration for periodic search of type LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE. */
struct lte_lc_periodic_search_range_cfg {
	/** Sleep time in seconds between searches in the beginning of the range.
	 *  Allowed values: 0 - 65535 seconds.
	 */
	uint16_t initial_sleep;

	/** Sleep time in seconds between searches in the end of the range.
	 *  Allowed values: 0 - 65535 seconds.
	 */
	uint16_t final_sleep;

	/** Optional target time in minutes for achieving the @c final_sleep value.
	 *  This can be used to determine angle factor between the initial and final sleep times.
	 *  The timeline for the @c time_to_final_sleep starts from the beginning of the search
	 *  pattern.
	 *  If given, the value cannot be greater than the value of the @c pattern_end_point value
	 *  in the same search pattern.
	 *  If not given, the angle factor is calculated by using the @c pattern_end_point value so
	 *  that the @c final_sleep value is reached at the point of @c pattern_end_point.
	 *
	 *  Allowed values:
	 *  -1: Not used
	 *   0 - 1080 minutes
	 */
	int16_t time_to_final_sleep;

	/** Time in minutes that must elapse before entering the next search pattern. The timeline
	 *  for @c pattern_end_point starts from the beginning of the limited service starting
	 *  point, which is the moment when the first sleep period started.
	 *
	 *  Allowed values: 0 - 1080 minutes.
	 */
	int16_t pattern_end_point;
};

/** Configuration for periodic search of type LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE.
 *  1 to 5 sleep time values for sleep between searches can be configured.
 *  It's mandatory to provide @c val_1, while the rest are optional.
 *  Unused values must be set to -1.
 *  After going through all values, the last value of the last search pattern is repeated, if
 *  not configured differently by the @c loop or @c return_to_pattern parameters.
 *
 *  Allowed values:
 *  -1: Value unused.
 *   0 - 65535 seconds.
 */
struct lte_lc_periodic_search_table_cfg {
	/** Mandatory when LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE is used. */
	int val_1;

	/** Optional sleep time.
	 *  Must be set to -1 if not used.
	 */
	int val_2;

	/** Optional sleep time. @c val_2 must be configured for this parameter to have effect.
	 *  Must be set to -1 if not used.
	 */
	int val_3;

	/** Optional sleep time. @c val_3 must be configured for this parameter to have effect.
	 *  Must be set to -1 if not used.
	 */
	int val_4;

	/** Optional sleep time. @c val_4 must be configured for this parameter to have effect.
	 *  Must be set to -1 if not used.
	 */
	int val_5;
};

/** @brief Periodic search pattern.
 *	   A search pattern may be of either 'range' or 'table' type.
 */
struct lte_lc_periodic_search_pattern {
	enum lte_lc_periodic_search_pattern_type {
		LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE = 0,
		LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE = 1,
	} type;

	union {
		struct lte_lc_periodic_search_range_cfg range;
		struct lte_lc_periodic_search_table_cfg table;
	};
};

/** @brief Periodic search configuration. */
struct lte_lc_periodic_search_cfg {
	/** Indicates if the last given pattern is looped from the beginning
	 *  when the pattern has ended.
	 *  If several patterns are configured, this impacts only the last pattern.
	 */
	bool loop;

	/** Indicates if the modem can return to a given search pattern with shorter sleeps, for
	 *  example, when radio conditions change and the given pattern index has already
	 *  been exceeded.
	 *
	 *  Allowed values:
	 *  0: No return pattern.
	 *  1 - 4: Return to search pattern index 1..4.
	 */
	uint16_t return_to_pattern;

	/** 0: No optimization. Every periodic search shall be all band search.
	 *  1: Use default optimizations predefined by modem. Predefinition depends on
	 *     the active data profile.
	 *     See "nRF91 AT Commands - Command Reference Guide" for more information.
	 *  2 - 20: Every n periodic search must be an all band search.
	 */
	uint16_t band_optimization;

	/** The number of valid patterns. Range 1 - 4. */
	size_t pattern_count;

	/** Array of periodic search patterns. */
	struct lte_lc_periodic_search_pattern patterns[4];
};

/**
 * @brief Link controller callback for modem functional mode changes.
 */
struct lte_lc_cfun_cb {
	void (*callback)(enum lte_lc_func_mode, void *ctx);
	void *context;
};

/**
 * @brief Define a callback for functional mode changes through @ref lte_lc_func_mode_set.
 *
 * @param name Callback name
 * @param _callback Callback function
 * @param _context User-defined context
 */
#define LTE_LC_ON_CFUN(name, _callback, _context)                                                  \
	static void _callback(enum lte_lc_func_mode, void *ctx);                                   \
	STRUCT_SECTION_ITERABLE(lte_lc_cfun_cb, lte_lc_cfun_cb_##name) = {                         \
		.callback = _callback,                                                             \
		.context = _context,                                                               \
	};

struct lte_lc_evt {
	enum lte_lc_evt_type type;
	union {
		enum lte_lc_nw_reg_status nw_reg_status;
		enum lte_lc_rrc_mode rrc_mode;
		struct lte_lc_psm_cfg psm_cfg;
		struct lte_lc_edrx_cfg edrx_cfg;
		struct lte_lc_cell cell;
		enum lte_lc_lte_mode lte_mode;
		struct lte_lc_modem_sleep modem_sleep;
		enum lte_lc_modem_evt modem_evt;

		/* Time until next Tracking Area Update in milliseconds. */
		uint64_t time;

		struct lte_lc_cells_info cells_info;
	};
};

typedef void(*lte_lc_evt_handler_t)(const struct lte_lc_evt *const evt);

/** @brief Register event handler for LTE events.
 *
 *  @param handler Event handler.
 */

void lte_lc_register_handler(lte_lc_evt_handler_t handler);

/**
 * @brief Function to de-register event handler for LTE events.
 *
 *  @param handler Event handler.
 *
 * @retval 0            If command execution was successful.
 * @retval -ENXIO       If handler cannot be found.
 * @retval -EINVAL      If handler is a NULL pointer.
 */
int lte_lc_deregister_handler(lte_lc_evt_handler_t handler);

/** @brief Initializes the module and configures the modem.
 *
 * @note a follow-up call to lte_lc_connect() or lte_lc_connect_async() must be
 *	 made to establish an LTE connection. The module can be initialized
 *	 only once, and subsequent calls will return 0.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_init(void);

/** @brief Function to make a connection with the modem.
 *
 * @note Prior to calling this function a call to @ref lte_lc_init
 *	 must be made, otherwise -EPERM is returned.
 *
 * @note After initialization, the system mode will be set to the default mode
 *	 selected with Kconfig and LTE preference set to automatic selection.
 *
 * @retval 0 if successful.
 * @retval -EPERM if the link controller was not initialized.
 * @retval -EFAULT if an AT command failed.
 * @retval -ETIMEDOUT if a connection attempt timed out before the device was
 *	   registered to a network.
 * @retval -EINPROGRESS if a connection establishment attempt is already in progress.
 */
int lte_lc_connect(void);

/** @brief Initializes the LTE module, configures the modem and connects to LTE
 *	   network. The function blocks until connection is established, or
 *	   the connection attempt times out.
 *
 * @note The module can be initialized only once, and repeated calls will
 *	 return 0. lte_lc_connect_async() should be used on subsequent
 *	 calls.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 * @retval -ETIMEDOUT if a connection attempt timed out before the device was
 *	   registered to a network.
 * @retval -EINPROGRESS if a connection establishment attempt is already in progress.
 */
int lte_lc_init_and_connect(void);

/** @brief Connect to LTE network. Non-blocking.
 *
 * @note The module must be initialized before this function is called.
 *
 * @param handler Event handler for receiving LTE events. The parameter can be
 *	          NULL if an event handler is already registered.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if no event handler was registered.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_connect_async(lte_lc_evt_handler_t handler);

/** @brief Initializes the LTE module, configures the modem and connects to LTE
 *	   network. Non-blocking.
 *
 * @note The module can be initialized only once, and repeated calls will
 *	 return 0. lte_lc_connect() should be used on subsequent calls.
 *
 * @param handler Event handler for receiving LTE events. The parameter can be
 *		  NULL if an event handler is already registered.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 * @retval -EINVAL if no event handler was registered.
 */
int lte_lc_init_and_connect_async(lte_lc_evt_handler_t handler);

/** @brief Deinitialize the LTE module, powers of the modem.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_deinit(void);

/** @brief Function for sending the modem to offline mode
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_offline(void);

/** @brief Function for sending the modem to power off mode
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_power_off(void);

/** @brief Function for sending the modem to normal mode
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_normal(void);

/** @brief Function for setting modem PSM parameters:
 *         requested periodic TAU (RPTAU) and requested active time (RAT)
 *         to be used when PSM mode is subsequently enabled using `lte_lc_psm_req`.
 *         For reference see 3GPP 27.007 Ch. 7.38.
 *
 * @param rptau Requested periodic TAU as null-terminated string.
 *              Set NULL to use manufacturer-specific default value.
 * @param rat Requested active time as null-terminated string.
 *            Set NULL to use manufacturer-specific default value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_psm_param_set(const char *rptau, const char *rat);

/** @brief Function for requesting modem to enable or disable
 *         power saving mode (PSM) using default Kconfig value or as set using
 *         `lte_lc_psm_param_set`.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_psm_req(bool enable);

/** @brief Function for getting the current PSM (Power Saving Mode)
 *	   configurations for periodic TAU (Tracking Area Update) and
 *	   active time, both in units of seconds.
 *
 * @param tau Pointer to the variable for parsed periodic TAU interval in
 *	      seconds. Positive integer, or -1 if timer is deactivated.
 * @param active_time Pointer to the variable for parsed active time in seconds.
 *	              Positive integer, or -1 if timer is deactivated.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EBADMSG if no active time and/or TAU value was received, including the case when
 *         modem is not registered to network.
 */
int lte_lc_psm_get(int *tau, int *active_time);

/** @brief Function for setting Paging Time Window (PTW) value to be used when
 *	   eDRX is requested using `lte_lc_edrx_req`. PTW is set individually
 *	   for LTE-M and NB-IoT.
 *	   Requesting a specific PTW configuration should be done with caution.
 *	   The requested value must be compliant with the eDRX value that is
 *	   configured, and it's usually best to let the modem use default PTW
 *	   values.
 *	   For reference to which values can be set, see subclause 10.5.5.32 of 3GPP TS 24.008.
 *
 * @param mode LTE mode to which the PTW value applies.
 * @param ptw Paging Time Window value as null-terminated string.
 *            Set NULL to use manufacturer-specific default value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_ptw_set(enum lte_lc_lte_mode mode, const char *ptw);

/** @brief Function for setting modem eDRX value to be used when
 *         eDRX is subsequently enabled using `lte_lc_edrx_req`.
 *         For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @param mode LTE mode to which the eDRX value applies.
 * @param edrx eDRX value as null-terminated string.
 *             Set NULL to use manufacturer-specific default.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx);

/** @brief Function for requesting modem to enable or disable
 *         use of eDRX using values set by `lte_lc_edrx_param_set`. The
 *         default values are defined in Kconfig.
 *         For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @param enable Boolean value enabling or disabling the use of eDRX.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_edrx_req(bool enable);

/** @brief Function for setting modem RAI value to be used when
 *         RAI is subsequently enabled using `lte_lc_rai_req`.
 *         For reference see 3GPP 24.301 Ch. 9.9.4.25.
 *
 * @param value RAI value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_rai_param_set(const char *value);

/** @brief Function for requesting modem to enable or disable
 *         use of RAI using values set by `lte_lc_rai_param_set`. The
 *         default values are defined in Kconfig.
 *
 * @param enable Boolean value enabling or disabling the use of RAI.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 * @retval -EOPNOTSUPP if RAI is not supported in the current system mode.
 */
int lte_lc_rai_req(bool enable);

/** @brief Get the current network registration status.
 *
 * @param status Pointer for network registration status.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the network registration could not be retrieved from the modem.
 */
int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status);

/** @brief Set the modem's system mode and LTE preference.
 *
 * @param mode System mode to set.
 * @param preference System mode preference.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the network registration could not be retrieved from the modem.
 */
int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference);

/** @brief Get the modem's system mode and LTE preference.
 *
 * @param mode Pointer to system mode variable.
 * @param preference Pointer to system mode preference variable. Can be NULL.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the system mode could not be retrieved from the modem.
 */
int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference);

/** @brief Set the modem's functional mode.
 *
 * @param mode Functional mode to set.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the functional mode could not be retrieved from the modem.
 */
int lte_lc_func_mode_set(enum lte_lc_func_mode mode);

/** @brief Get the modem's functional mode.
 *
 * @param mode Pointer to functional mode variable.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the functional mode could not be retrieved from the modem.
 */
int lte_lc_func_mode_get(enum lte_lc_func_mode *mode);

/** @brief Get the currently active LTE mode.
 *
 * @param mode Pointer to LTE mode variable.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the current LTE mode could not be retrieved.
 * @retval -EBADMSG if the LTE mode was not recognized.
 */
int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode);

/** @brief Initiate a neighbor cell measurement.
 *         The result of the measurement is reported back as an event of the type
 *         LTE_LC_EVT_NEIGHBOR_CELL_MEAS, meaning that an event handler must be
 *         registered to receive the information.
 *	   Depending on the network conditions, LTE connection state and requested
 *         search type, it may take a while before the measurement result is
 *         ready and reported back.
 *         After the event is received, the neighbor cell measurements are automatically stopped.
 *
 * @note For modem firmware versions < v1.3.1, LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT
 *       is the only accepted type. Other types will result in an error being returned.
 *       For modem firmware versions >= v1.3.4, also GCI search types and gci_count
 *       are accepted.
 *
 * @param params Pointer to search type parameters or NULL to initiate a measurement
 *               with the default parameters.
 *               See @c struct lte_lc_ncellmeas_params for more information.
 *
 * @retval 0 if neighbor cell measurement was successfully initiated.
 * @retval -EFAULT if AT command failed.
 * @retval -EINPROGRESS if a neighbor cell measurement is already in progress.
 */
int lte_lc_neighbor_cell_measurement(struct lte_lc_ncellmeas_params *params);

/** @brief Cancel an ongoing neighbor cell measurement.
 *
 *
 * @retval 0 if neighbor cell measurement was cancelled.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_neighbor_cell_measurement_cancel(void);

/** @brief Get connection evaluation parameters. Connection evaluation parameters can be used to
 *	   determine the energy efficiency of data transmission prior to the actual
 *	   data transmission.
 *
 * @param params Pointer to structure to hold connection evaluation parameters.
 *
 * @return Zero on success, negative errno code if the API call fails, and a positive error
 *         code if the API call succeeds but connection evaluation fails due to modem/network
 *         related reasons.
 *
 * @retval 0 Evaluation succeeded.
 * @retval 1 Evaluation failed, no cell available.
 * @retval 2 Evaluation failed, UICC not available.
 * @retval 3 Evaluation failed, only barred cells available.
 * @retval 4 Evaluation failed, radio busy (e.g GNSS activity)
 * @retval 5 Evaluation failed, aborted due to higher priority operation.
 * @retval 6 Evaluation failed, UE not registered to network.
 * @retval 7 Evaluation failed, Unspecified.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EOPNOTSUPP if connection evaluation is not available in the current functional mode.
 * @retval -EBADMSG if parsing of the AT command response failed.
 */
int lte_lc_conn_eval_params_get(struct lte_lc_conn_eval_params *params);

/** @brief Enable modem domain events. See @ref lte_lc_modem_evt for
 *	   more information on what events may be received.
 *
 *  @note This feature is supported for nRF9160 modem firmware v1.3.0 and later
 *	  versions. Attempting to use this API with older modem versions will
 *	  result in an error being returned.
 *
 *  @note An event handler must be registered in order to receive events.
 *
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_modem_events_enable(void);

/** @brief Disable modem domain events.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_modem_events_disable(void);

/** @brief Configure periodic searches. This configuration affects the periodic searches
 *	   that the modem performs in limited service state to obtain normal service.
 *	   See @c struct lte_lc_periodic_search_cfg and
 *	   "nRF91 AT Commands - Command Reference Guide" for more information and
 *	   in-depth explanations of periodic search configuration.
 *
 * @retval 0 if the configuration was successfully sent to the modem.
 * @retval -EINVAL if an input parameter was NULL or contained an invalid pattern count.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 * @retval -EBADMSG if the modem responded with an error to an AT command.
 */
int lte_lc_periodic_search_set(const struct lte_lc_periodic_search_cfg *const cfg);

/** @brief Get the configured periodic search parameters.
 *
 * @retval 0 if a configuration was found and populated to the provided pointer.
 * @retval -EINVAL if input parameter was NULL.
 * @retval -ENOENT if no periodic search was not configured.
 * @retval -EFAULT if an AT command failed.
 * @retval -EBADMSG if the modem responded with an error to an AT command or the
 *		    response could not be parsed.
 */
int lte_lc_periodic_search_get(struct lte_lc_periodic_search_cfg *const cfg);

/** @brief Clear the configured periodic search parameters.
 *
 * @retval 0 if the configuration was cleared.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 * @retval -EBADMSG if the modem responded with an error to an AT command.
 */
int lte_lc_periodic_search_clear(void);

/** @brief Request an extra search. This can be used for example when modem is in
 *	   sleep state between periodic searches. The search is performed only when
 *	   the modem is in sleep state between periodic searches.
 *
 * @retval 0 if the search request was successfully delivered to the modem.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 */
int lte_lc_periodic_search_request(void);

/** @brief Read the current reduced mobility mode.
 *
 *  @note This feature is supported for nRF9160 modem firmware v1.3.2 and later
 *	  versions. Attempting to use this API with older modem versions will
 *	  result in an error being returned.
 *
 * @param[out] mode pointer to where the current reduced mobility mode should be written to
 *
 * @retval 0 if a mode was found and written to the provided pointer.
 * @retval -EINVAL if input parameter was NULL.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_reduced_mobility_get(enum lte_lc_reduced_mobility_mode *mode);

/** @brief Set reduced mobility mode.
 *
 *  @note This feature is supported for nRF9160 modem firmware v1.3.2 and later
 *	  versions. Attempting to use this API with older modem versions will
 *	  result in an error being returned.
 *
 * @param[in] mode new reduced mobility mode
 *
 * @retval 0 if the new reduced mobility mode was accepted by the modem.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_reduced_mobility_set(enum lte_lc_reduced_mobility_mode mode);

/** @brief Reset modem to factory settings.
 *	   This operation is only allowed when the modem is not activated.
 *
 *  @note This feature is supported for nRF9160 modem firmware v1.3.0 and later
 *	  versions. Attempting to use this API with older modem versions will
 *	  result in an error being returned.
 *
 * @param type Variable that determines what type of modem data will be reset.
 *
 * @retval 0 if factory reset was performed successfully.
 * @retval -EFAULT if an AT command failed.
 */
int lte_lc_factory_reset(enum lte_lc_factory_reset_type type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
