/**
 * @file lte_lc.h
 *
 * @brief Public APIs for the LTE Link Control driver.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_
#define ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_

/**
 * @file lte_lc.h
 *
 * @defgroup lte_lc LTE Link Controller
 *
 * @{
 *
 * @brief Public APIs for the LTE Link Controller.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	LTE_LC_EVT_NW_REG_STATUS,
	LTE_LC_EVT_PSM_UPDATE,
	LTE_LC_EVT_EDRX_UPDATE,
	LTE_LC_EVT_RRC_UPDATE,
	LTE_LC_EVT_CELL_UPDATE,

	/** The currently active LTE mode is updated. If a system mode that
	 *  enables both LTE-M and NB-IoT is configured, the modem may change
	 *  the currently active LTE mode based on the system mode preference
	 *  and network availability. This event will then indicate which
	 *  LTE mode is currently used by the modem.
	 */
	LTE_LC_EVT_LTE_MODE_UPDATE,

	/** Tracking Area Update pre-warning.
	 *  This event will be received a configurable amount of time before TAU is scheduled to
	 *  occur. This gives the application the opportunity to send data over the network before
	 *  the TAU happens, thus saving power by avoiding sending data and the TAU separately.
	 */
	LTE_LC_EVT_TAU_PRE_WARNING,

	/** Event containing results from neighbor cell measurements. */
	LTE_LC_EVT_NEIGHBOR_CELL_MEAS,

	/** Modem sleep pre-warning
	 *  This event will be received a configurable amount of time before the modem exits sleep.
	 *  The time parameter associated with this event signifies the time until modem exits
	 *  sleep.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING,

	/** This event will be received when the modem exits sleep. */
	LTE_LC_EVT_MODEM_SLEEP_EXIT,

	/** This event will be received when the modem enters sleep.
	 *  The time parameter associated with this event signifies the duration of the sleep.
	 */
	LTE_LC_EVT_MODEM_SLEEP_ENTER,
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

struct lte_lc_cell {
	/** Mobile Country Code. */
	int mcc;

	/** Mobile Network Code. */
	int mnc;

	/** E-UTRAN cell ID. */
	uint32_t id;

	/** Tracking area code. */
	uint32_t tac;

	/** EARFCN of the neighbour cell, per 3GPP TS 36.101. */
	uint32_t earfcn;

	/** Timing advance decimal value.
	 *  Range [0..20512, TIMING_ADVANCE_NOT_VALID = 65535].
	 */
	uint16_t timing_advance;

	/** Measurement time of serving cell in milliseconds.
	 *  Range 0 - 18 446 744 073 709 551 614 ms.
	 */
	uint64_t measurement_time;

	/** Physical cell ID. */
	uint16_t phys_cell_id;

	/** RSRP of the neighbor celll.
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

struct lte_lc_ncell {
	/** EARFCN of the neighbour cell, per 3GPP TS 36.101. */
	uint32_t earfcn;

	/** Difference in milliseconds of serving cell and neighbor cell
	 *  measurement, in the range -99999 ms < time_diff < 99999 ms.
	 */
	int time_diff;

	/** Physical cell ID. */
	uint16_t phys_cell_id;

	/** RSRP of the neighbor celll. Format is the same as for RSRP member
	 *  of struct lte_lc_cell.
	 */
	int16_t rsrp;

	/** RSRQ of the neighbor cell. Format is the same as for RSRQ member
	 *  of struct lte_lc_cell.
	 */
	int16_t rsrq;
};

/** @brief Structure containing results of neighbor cell measurements.
 *	   The ncells_count member indicates whether or not the structure contains
 *	   any valid cell information. If it is zero, no cells were found, and
 *	   the information in the rest of structure members do not contain valid data.
 */
struct lte_lc_cells_info {
	struct lte_lc_cell current_cell;
	uint8_t ncells_count;
	struct lte_lc_ncell *neighbor_cells;
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
	LTE_LC_CE_LEVEL_UNKNOWN			= UINT8_MAX,
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

		/* Time until next Tracking Area Update in milliseconds. */
		uint64_t time;

		struct lte_lc_cells_info cells_info;
	};
};

typedef void(*lte_lc_evt_handler_t)(const struct lte_lc_evt *const evt);

/** @brief Register event handler for LTE events.
 *
 *  @param handler Event handler. Handler is de-registered if parameter is
 *		   NULL.
 */
void lte_lc_register_handler(lte_lc_evt_handler_t handler);

/**@brief Initializes the module and configures the modem.
 *
 * @note a follow-up call to lte_lc_connect() or lte_lc_connect_async() must be
 *	 made to establish an LTE connection. The module can be initialized
 *	 only once, and subsequent calls will return -EALREADY.
 *
 * @return Zero on success or (negative) error code otherwise.
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
 * @return Zero on success, -EPERM if the module has not been initialized,
 *	   otherwise a (negative) error code.
 */
int lte_lc_connect(void);

/**@brief Initializes the LTE module, configures the modem and connects to LTE
 *	  network. The function blocks until connection is established, or
 *	  the connection attempt times out.
 *
 * @note The module can be initialized only once, and repeated calls will
 *	 return -EALREADY. lte_lc_connect_async() should be used on subsequent
 *	 calls.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_init_and_connect(void);

/**@brief Connect to LTE network. Non-blocking.
 *
 * @note The module must be initialized before this function is called.
 *
 * @param handler Event handler for receiving LTE events. The parameter can be
 *		  NULL if an event handler is already registered.
 *
 * @return Zero on success, -EINVAL if no handler is provided and not already
 *	   registered, otherwise a (negative) error code.
 */
int lte_lc_connect_async(lte_lc_evt_handler_t handler);

/**@brief Initializes the LTE module, configures the modem and connects to LTE
 *	  network. Non-blocking.
 *
 * @note The module can be initialized only once, and repeated calls will
 *	 return -EALREADY. lte_lc_connect() should be used on subsequent calls.
 *
 * @param handler Event handler for receiving LTE events. The parameter can be
 *		  NULL if an event handler is already registered.
 *
 * @return Zero on success, -EINVAL if no handler is provided and not already
 *	   registered, otherwise a (negative) error code.
 */
int lte_lc_init_and_connect_async(lte_lc_evt_handler_t handler);

/**@brief Deinitialize the LTE module, powers of the modem.
 *
 * @return Zero on success, -EIO if it fails.
 */
int lte_lc_deinit(void);

/** @brief Function for sending the modem to offline mode
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_offline(void);

/** @brief Function for sending the modem to power off mode
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_power_off(void);

/** @brief Function for sending the modem to normal mode
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_normal(void);

/** @brief Function for setting modem PSM parameters:
 * requested periodic TAU (RPTAU) and requested active time (RAT)
 * to be used when psm mode is subsequently enabled using `lte_lc_psm_req`.
 * For reference see 3GPP 27.007 Ch. 7.38.
 *
 * @param rptau Requested periodic TAU as null-terminated string.
 *        Set NULL to use manufacturer-specific default value.
 * @param rat Requested active time as null-terminated string.
 *         Set NULL to use manufacturer-specific default value.
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_psm_param_set(const char *rptau, const char *rat);

/** @brief Function for requesting modem to enable or disable
 * power saving mode (PSM) using default Kconfig value or as set using
 * `lte_lc_psm_param_set`.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_psm_req(bool enable);

/**@brief Function for getting the current PSM (Power Saving Mode)
 *	  configurations for periodic TAU (Tracking Area Update) and
 *	  active time, both in units of seconds.
 *
 * @param tau Pointer to the variable for parsed periodic TAU interval in
 *	      seconds. Positive integer, or -1 if timer is deactivated.
 * @param active_time Pointer to the variable for parsed active time in seconds.
 *		      Positive integer, or -1 if timer is deactivated.
 *
 * @return Zero on success or (negative) error code otherwise.
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
 *        Set NULL to use manufacturer-specific default value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_ptw_set(enum lte_lc_lte_mode mode, const char *ptw);

/** @brief Function for setting modem eDRX value to be used when
 * eDRX is subsequently enabled using `lte_lc_edrx_req`.
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @param mode LTE mode to which the eDRX value applies.
 * @param edrx eDRX value as null-terminated string.
 *        Set NULL to use manufacturer-specific default.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx);

/** @brief Function for requesting modem to enable or disable
 * use of eDRX using values set by `lte_lc_edrx_param_set`. The
 * default values are defined in kconfig.
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @param enable Boolean value enabling or disabling the use of eDRX.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_edrx_req(bool enable);


/** @brief Function for setting modem RAI value to be used when
 * RAI is subsequently enabled using `lte_lc_rai_req`.
 * For reference see 3GPP 24.301 Ch. 9.9.4.25.
 *
 * @param value RAI value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_rai_param_set(const char *value);

/** @brief Function for requesting modem to enable or disable
 * use of RAI using values set by `lte_lc_rai_param_set`. The
 * default values are defined in Kconfig.
 *
 * @param enable Boolean value enabling or disabling the use of RAI.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_rai_req(bool enable);

/**@brief Get the current network registration status.
 *
 * @param status Pointer for network registation status.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status);

/**@brief Set the modem's system mode and LTE preference.
 *
 * @param mode System mode to set.
 * @param preference System mode preference.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference);

/**@brief Get the modem's system mode and LTE preference.
 *
 * @param mode Pointer to system mode variable.
 * @param preference Pointer to system mode preference variable. Can be NULL.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference);

/**@brief Set the modem's functional mode.
 *
 * @param mode Functional mode to set.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_func_mode_set(enum lte_lc_func_mode mode);

/**@brief Get the modem's functional mode.
 *
 * @param mode Pointer to functional mode variable.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_func_mode_get(enum lte_lc_func_mode *mode);

/**@brief Get the currently active LTE mode.
 *
 * @param mode Pointer to LTE mode variable.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode);

/**@brief Initiate a neighbor cell measurement.
 *	  The result of the measurement is reported back as an event of the type
 *	  LTE_LC_EVT_NEIGHBOR_CELL_MEAS, meaning that an event handler must be
 *	  registered to receive the information.
 *	  Depending on the network conditions and LTE connection state, it may
 *	  take a while before the measurement result is ready and reported back.
 *	  After the event is received, the neighbor cell measurements
 *	  are automatically stopped.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_neighbor_cell_measurement(void);

/**@brief Cancel an ongoing neighbor cell measurement.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_neighbor_cell_measurement_cancel(void);

/**@brief Get connection evaluation parameters. Connection evaluation parameters can be used to
 *	  determine the energy efficiency of data transmission prior to the actual
 *	  data transmission.
 *
 * @param params Pointer to structure to hold connection evaluation parameters.
 *
 * @return Zero on success, negative errno code if the API call fails, and a positive error
 *         code if the API call succeeds but connection evalution fails due to modem/network related
 *         reasons.
 *
 * @retval 0 Evaluation succeeded.
 * @retval 1 Evaluation failed, no cell available.
 * @retval 2 Evaluation failed, UICC not available.
 * @retval 3 Evaluation failed, only barred cells available.
 * @retval 4 Evaluation failed, radio busy (e.g GNSS activity)
 * @retval 5 Evaluation failed, aborted due to higher priority operation.
 * @retval 6 Evaluation failed, UE not registered to network.
 * @retval 7 Evaluation failed, Unspecified.
 */
int lte_lc_conn_eval_params_get(struct lte_lc_conn_eval_params *params);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
