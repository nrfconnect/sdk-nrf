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
#include <zephyr/net/net_ip.h>

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
 * @brief Network registration status.
 *
 * @note Maps directly to the registration status as returned by the AT command `AT+CEREG?`.
 */
enum lte_lc_nw_reg_status {
	/** Not registered. UE is not currently searching for an operator to register to. */
	LTE_LC_NW_REG_NOT_REGISTERED			= 0,

	/** Registered, home network. */
	LTE_LC_NW_REG_REGISTERED_HOME			= 1,

	/**
	 * Not registered, but UE is currently trying to attach or searching for an operator
	 * to register to.
	 */
	LTE_LC_NW_REG_SEARCHING				= 2,

	/** Registration denied. */
	LTE_LC_NW_REG_REGISTRATION_DENIED		= 3,

	/** Unknown, for example out of LTE coverage. */
	LTE_LC_NW_REG_UNKNOWN				= 4,

	/** Registered, roaming. */
	LTE_LC_NW_REG_REGISTERED_ROAMING		= 5,

	/**
	 * Not registered. UE is not currently searching for an operator to register to.
	 * Device in receive only mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_RX_ONLY_NOT_REGISTERED		= 50,

	/**
	 * Registered in stored cellular profile, home network. Device in receive only mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_RX_ONLY_REGISTERED_HOME		= 51,

	/**
	 * Not registered, but UE is currently trying to attach or searching for an operator
	 * to register to. Device in receive only mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_RX_ONLY_SEARCHING			= 52,

	/**
	 * Registration denied. Device in receive only mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_RX_ONLY_REGISTRATION_DENIED	= 53,

	/**
	 * Unknown, for example, out of LTE coverage. Device in receive only mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_RX_ONLY_UNKNOWN			= 54,

	/**
	 * Registered in stored cellular profile, roaming. Device in receive only mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_RX_ONLY_REGISTERED_ROAMING	= 55,

	/** Not registered due to UICC failure. */
	LTE_LC_NW_REG_UICC_FAIL				= 90,

	/**
	 * The modem has completed searches, but no suitable cell for normal service was
	 * found.
	 * This may be used as a trigger for changing the configured system mode.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_NW_REG_NO_SUITABLE_CELL			= 91
};

/** @brief System mode. */
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

	/**
	 * NTN NB-IoT only.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_SYSTEM_MODE_NTN_NBIOT
};

/**
 * @brief LTE mode.
 *
 * The values for LTE-M and NB-IoT correspond to the values for the access technology field in AT
 * commands.
 */
enum lte_lc_lte_mode {
	/** None. */
	LTE_LC_LTE_MODE_NONE		= 0,

	/** LTE-M. */
	LTE_LC_LTE_MODE_LTEM		= 7,

	/** NB-IoT. */
	LTE_LC_LTE_MODE_NBIOT		= 9,
	/**
	 * NTN NB-IoT.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_LTE_MODE_NTN_NBIOT	= 14
};

/**
 * @brief LTE mode preference.
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
 * @brief Functional mode, used to control RF functionality in the modem.
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
	LTE_LC_FUNC_MODE_POWER_OFF			= 0,

	/**
	 * Sets the device to full functionality.
	 *
	 * Both LTE and GNSS will become active if the respective system modes are enabled.
	 */
	LTE_LC_FUNC_MODE_NORMAL				= 1,

	/**
	 * Sets the device to receive only functionality.
	 *
	 * Features supported by mfw_nrf9160 in receive only mode:
	 * - lte_lc_conn_eval_params_get()
	 *
	 * Features supported by mfw_nrf91x1 and mfw_nrf9151-ntn in receive only mode:
	 * - Network selection
	 * - lte_lc_conn_eval_params_get()
	 * - lte_lc_neighbor_cell_measurement()
	 * - lte_lc_env_eval()
	 *
	 * mfw_nrf91x1 and mfw_nrf9151-ntn perform network selection in receive only mode.
	 * When the search has been completed, an @ref LTE_LC_EVT_MODEM_EVENT is sent with modem
	 * event @ref LTE_LC_MODEM_EVT_SEARCH_DONE. The application can check the last received
	 * @ref LTE_LC_EVT_CELL_UPDATE to see if a suitable cell was found or not.
	 *
	 * After network selection, device remains camped on the found cell, but will not try to
	 * register unless the functional mode is changed. LTE registration can be triggered on
	 * the cell by setting the functional mode to @ref LTE_LC_FUNC_MODE_NORMAL or
	 * @ref LTE_LC_FUNC_MODE_ACTIVATE_LTE. Device should not be left in receive only mode for
	 * longer than necessary, because current consumption will be elevated.
	 */
	LTE_LC_FUNC_MODE_RX_ONLY			= 2,

	/**
	 * Sets the device to flight mode.
	 *
	 * Disables both transmit and receive RF circuits and deactivates LTE and GNSS services.
	 */
	LTE_LC_FUNC_MODE_OFFLINE			= 4,

	/** Deactivates LTE without shutting down GNSS services. */
	LTE_LC_FUNC_MODE_DEACTIVATE_LTE			= 20,

	/** Activates LTE without changing GNSS. */
	LTE_LC_FUNC_MODE_ACTIVATE_LTE			= 21,

	/** Deactivates GNSS without shutting down LTE services. */
	LTE_LC_FUNC_MODE_DEACTIVATE_GNSS		= 30,

	/** Activates GNSS without changing LTE. */
	LTE_LC_FUNC_MODE_ACTIVATE_GNSS			= 31,

	/** Deactivates UICC. */
	LTE_LC_FUNC_MODE_DEACTIVATE_UICC		= 40,

	/** Activates UICC. */
	LTE_LC_FUNC_MODE_ACTIVATE_UICC			= 41,

	/** Sets the device to flight mode without shutting down UICC. */
	LTE_LC_FUNC_MODE_OFFLINE_UICC_ON		= 44,

	/**
	 * Sets the device to flight mode while preserving the LTE registration context.
	 *
	 * Used to change the active cellular profile when using a dual UICC solution where one
	 * of the UICCs is a SoftSIM. Allowed only when two cellular profiles have been configured.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG		= 45,

	/**
	 * Sets the device to flight mode while preserving the LTE registration context and
	 * without shutting down the UICC.
	 *
	 * Used to change the active cellular profile when using an eUICC solution with dual
	 * profiles. Allowed only when two cellular profiles have been configured.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG_UICC_ON	= 46
};

/** @brief Event type. */
enum lte_lc_evt_type {
#if defined(CONFIG_LTE_LC_NETWORK_REGISTRATION_MODULE)
	/**
	 * Network registration status.
	 *
	 * The associated payload is the @c lte_lc_evt.nw_reg_status member of type
	 * @ref lte_lc_nw_reg_status in the event.
	 */
	LTE_LC_EVT_NW_REG_STATUS		= 0,
#endif /* CONFIG_LTE_LC_NETWORK_REGISTRATION_MODULE */

#if defined(CONFIG_LTE_LC_PSM_MODULE)
	/**
	 * PSM parameters provided by the network.
	 *
	 * The associated payload is the @c lte_lc_evt.psm_cfg member of type
	 * @ref lte_lc_psm_cfg in the event.
	 */
	LTE_LC_EVT_PSM_UPDATE			= 1,
#endif /* CONFIG_LTE_LC_PSM_MODULE */

#if defined(CONFIG_LTE_LC_EDRX_MODULE)
	/**
	 * eDRX parameters provided by the network.
	 *
	 * The associated payload is the @c lte_lc_evt.edrx_cfg member of type
	 * @ref lte_lc_edrx_cfg in the event.
	 */
	LTE_LC_EVT_EDRX_UPDATE			= 2,
#endif /* CONFIG_LTE_LC_EDRX_MODULE */

	/**
	 * RRC connection state.
	 *
	 * The associated payload is the @c lte_lc_evt.rrc_mode member of type
	 * @ref lte_lc_rrc_mode in the event.
	 */
	LTE_LC_EVT_RRC_UPDATE			= 3,

	/**
	 * Current cell.
	 *
	 * The associated payload is the @c lte_lc_evt.cell member of type
	 * @ref lte_lc_cell in the event. Only the @c lte_lc_cell.tac and @c lte_lc_cell.id
	 * members are populated in the event payload. The rest are expected to be zero.
	 */
	LTE_LC_EVT_CELL_UPDATE			= 4,

	/**
	 * Current LTE mode.
	 *
	 * If a system mode that enables both LTE-M and NB-IoT is configured, the modem may change
	 * the currently active LTE mode based on the system mode preference and network
	 * availability. This event will then indicate which LTE mode is currently used by the
	 * modem.
	 *
	 * The associated payload is the @c lte_lc_evt.lte_mode member of type
	 * @ref lte_lc_lte_mode in the event.
	 */
	LTE_LC_EVT_LTE_MODE_UPDATE		= 5,

#if defined(CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE)
	/**
	 * Tracking Area Update pre-warning.
	 *
	 * This event will be received some time before TAU is scheduled to occur. The time is
	 * configurable. This gives the application the opportunity to send data over the network
	 * before the TAU happens, thus saving power by avoiding sending data and the TAU
	 * separately.
	 *
	 * The associated payload is the @c lte_lc_evt.time member of type @c uint64_t in the event.
	 */
	LTE_LC_EVT_TAU_PRE_WARNING		= 6,
#endif /* CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE */

#if defined(CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE)
	/**
	 * Neighbor cell measurement results.
	 *
	 * The associated payload is the @c lte_lc_evt.cells_info member of type
	 * @ref lte_lc_cells_info in the event. In case of an error or if no cells were found,
	 * the @c lte_lc_cells_info.current_cell member is set to
	 * @ref LTE_LC_CELL_EUTRAN_ID_INVALID, and @c lte_lc_cells_info.ncells_count and
	 * @c lte_lc_cells_info.gci_cells_count members are set to zero.
	 */
	LTE_LC_EVT_NEIGHBOR_CELL_MEAS		= 7,
#endif /* CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE */

#if defined(CONFIG_LTE_LC_MODEM_SLEEP_MODULE)
	/**
	 * Modem sleep pre-warning.
	 *
	 * This event will be received some time before the modem exits sleep. The time is
	 * configurable.
	 *
	 * The associated payload is the @c lte_lc_evt.modem_sleep member of type
	 * @ref lte_lc_modem_sleep in the event. The @c lte_lc_modem_sleep.time parameter indicates
	 * the time until modem exits sleep.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING	= 8,

	/**
	 * Modem exited from sleep.
	 *
	 * The associated payload is the @c lte_lc_evt.modem_sleep member of type
	 * @ref lte_lc_modem_sleep in the event.
	 */
	LTE_LC_EVT_MODEM_SLEEP_EXIT		= 9,

	/**
	 * Modem entered sleep.
	 *
	 * The associated payload is the @c lte_lc_evt.modem_sleep member of type
	 * @ref lte_lc_modem_sleep in the event. The @c lte_lc_modem_sleep.time parameter indicates
	 * the duration of the sleep.
	 */
	LTE_LC_EVT_MODEM_SLEEP_ENTER		= 10,
#endif /* CONFIG_LTE_LC_MODEM_SLEEP_MODULE */

	/**
	 * Information about modem operation.
	 *
	 * The associated payload is the @c lte_lc_evt.modem_evt member of type
	 * @ref lte_lc_modem_evt in the event.
	 */
	LTE_LC_EVT_MODEM_EVENT			= 11,

#if defined(CONFIG_LTE_LC_RAI_MODULE)
	/**
	 * Information about RAI (Release Assistance Indication) configuration.
	 *
	 * The associated payload is the @c lte_lc_evt.rai_cfg member of type
	 * @ref lte_lc_rai_cfg in the event.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf91x1 v2.0.2 or later
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_EVT_RAI_UPDATE			= 12,
#endif /* CONFIG_LTE_LC_RAI_MODULE */

#if defined(CONFIG_LTE_LC_ENV_EVAL_MODULE)
	/**
	 * Environment evaluation result.
	 *
	 * The associated payload is the @c lte_lc_evt.env_eval_result member of type
	 * @ref lte_lc_env_eval_result in the event.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf91x1 v2.0.3 or later
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_EVT_ENV_EVAL_RESULT		= 13,
#endif /* CONFIG_LTE_LC_ENV_EVAL_MODULE */

#if defined(CONFIG_LTE_LC_PDN_MODULE)
	/**
	 * PDN event
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN				= 14,
#endif /* CONFIG_LTE_LC_PDN_MODULE */

#if defined(CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE)
	/**
	 * A cellular profile is activated for the current access technology.
	 *
	 * The associated payload is the @c lte_lc_evt.cellular_profile member of type
	 * @ref lte_lc_cellular_profile_evt in the event.
	 */
	LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE	= 15,
#endif /* CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE */
};

/** @brief RRC connection state. */
enum lte_lc_rrc_mode {
	/** Idle. */
	LTE_LC_RRC_MODE_IDLE		= 0,

	/** Connected. */
	LTE_LC_RRC_MODE_CONNECTED	= 1,
};

/** @brief Power Saving Mode (PSM) configuration. */
struct lte_lc_psm_cfg {
	/** @brief Periodic Tracking Area Update interval in seconds. */
	int tau;

	/**
	 * @brief Active-time (time from RRC idle to PSM) in seconds or @c -1 if PSM is
	 * deactivated.
	 */
	int active_time;
};

/** @brief eDRX configuration. */
struct lte_lc_edrx_cfg {
	/**
	 * @brief LTE mode for which the configuration is valid.
	 *
	 * If the mode is @ref LTE_LC_LTE_MODE_NONE, access technology is not using eDRX.
	 */
	enum lte_lc_lte_mode mode;

	/** @brief eDRX interval in seconds. */
	float edrx;

	/** @brief Paging time window in seconds. */
	float ptw;
};

/** @brief Maximum timing advance value. */
#define LTE_LC_CELL_TIMING_ADVANCE_MAX		20512
/** @brief Invalid timing advance value. */
#define LTE_LC_CELL_TIMING_ADVANCE_INVALID	65535
/** @brief Maximum EARFCN value. */
#define LTE_LC_CELL_EARFCN_MAX			262143
/** @brief Unknown or undetectable RSRP value. */
#define LTE_LC_CELL_RSRP_INVALID		255
/** @brief Unknown or undetectable RSRQ value. */
#define LTE_LC_CELL_RSRQ_INVALID		255
/** @brief Cell ID value not valid. */
#define LTE_LC_CELL_EUTRAN_ID_INVALID		UINT32_MAX
/** @brief Maximum cell ID value. */
#define LTE_LC_CELL_EUTRAN_ID_MAX		268435455
/** @brief Tracking Area Code value not valid. */
#define LTE_LC_CELL_TAC_INVALID			UINT32_MAX
/** @brief Time difference not valid. */
#define LTE_LC_CELL_TIME_DIFF_INVALID		0

/** @brief Neighbor cell information. */
struct lte_lc_ncell {
	/** @brief EARFCN per 3GPP TS 36.101. */
	uint32_t earfcn;

	/**
	 * @brief Difference of current cell and neighbor cell measurement in milliseconds,
	 * in the range -99999 ms < time_diff < 99999 ms. @ref LTE_LC_CELL_TIME_DIFF_INVALID
	 * if the value is not valid.
	 */
	int time_diff;

	/** @brief Physical cell ID. */
	uint16_t phys_cell_id;

	/**
	 * @brief RSRP.
	 *
	 * Format is the same as for @c lte_lc_cell.rsrp member.
	 */
	int16_t rsrp;

	/**
	 * @brief RSRQ.
	 *
	 * Format is the same as for @c lte_lc_cell.rsrq member.
	 */
	int16_t rsrq;
};

/** @brief Cell information. */
struct lte_lc_cell {
	/** @brief Mobile Country Code. */
	int mcc;

	/** @brief Mobile Network Code. */
	int mnc;

	/** @brief E-UTRAN cell ID, range 0 - @ref LTE_LC_CELL_EUTRAN_ID_MAX. */
	uint32_t id;

	/** @brief Tracking area code. */
	uint32_t tac;

	/** @brief EARFCN per 3GPP TS 36.101. */
	uint32_t earfcn;

	/**
	 * @brief Timing advance decimal value in basic time units (Ts).
	 *
	 * Ts = 1/(15000 x 2048) seconds (as specified in 3GPP TS 36.211).
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
	 * @brief Timing advance measurement time in milliseconds, calculated from modem boot time.
	 *
	 * Range 0 - 18 446 744 073 709 551 614 ms.
	 */
	uint64_t timing_advance_meas_time;

	/**
	 * @brief Cell measurement time in milliseconds, calculated from modem boot time.
	 *
	 * Range 0 - 18 446 744 073 709 551 614 ms.
	 */
	uint64_t measurement_time;

	/** @brief Physical cell ID. */
	uint16_t phys_cell_id;

	/**
	 * @brief RSRP.
	 *
	 * Can be converted into dBm using @ref RSRP_IDX_TO_DBM macro.
	 *
	 * * -17: RSRP < -156 dBm
	 * * -16: -156 ≤ RSRP < -155 dBm
	 * * ...
	 * * -3: -143 ≤ RSRP < -142 dBm
	 * * -2: -142 ≤ RSRP < -141 dBm
	 * * -1: -141 ≤ RSRP < -140 dBm
	 * * 0: Not used.
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
	 * @brief RSRQ.
	 *
	 * Can be converted into dB using @ref RSRQ_IDX_TO_DB macro.
	 *
	 * * -30: RSRQ < -34.5 dB
	 * * -29: -34 ≤ RSRQ < -33.5 dB
	 * * ...
	 * * -2: -20.5 ≤ RSRQ < -20 dB
	 * * -1: -20 ≤ RSRQ < -19.5 dB
	 * * 0: Not used.
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


/** @brief Results of neighbor cell measurements. */
struct lte_lc_cells_info {
	/**
	 * @brief The current cell information is valid if the current cell ID is not set to
	 * @ref LTE_LC_CELL_EUTRAN_ID_INVALID.
	 */
	struct lte_lc_cell current_cell;

	/**
	 * @brief Indicates whether or not the @c neighbor_cells contains valid neighbor cell
	 *  information. If it is zero, no neighbor cells were found for the current cell.
	 */
	uint8_t ncells_count;

	/** @brief Neighbor cells for the current cell. */
	struct lte_lc_ncell *neighbor_cells;

	/**
	 * @brief Indicates whether or not the @c gci_cells contains valid surrounding cell
	 * information from GCI search types (@ref lte_lc_neighbor_search_type).
	 * If it is zero, no cells were found.
	 */
	uint8_t gci_cells_count;

	/** @brief Surrounding cells found by the GCI search types. */
	struct lte_lc_cell *gci_cells;
};

/** @brief Modem sleep type. */
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
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf91x1
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM	= 7,
};

/** @brief Modem sleep information. */
struct lte_lc_modem_sleep {
	/** @brief Sleep type. */
	enum lte_lc_modem_sleep_type type;

	/**
	 * @brief Sleep time in milliseconds. If this value is set to -1,
	 * the sleep is considered infinite.
	 */
	int64_t time;
};

/** @brief Energy consumption estimate. */
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

/** @brief Cell in Tracking Area Identifier list. */
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

/** @brief CE level. */
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

/** @brief Modem domain event type. */
enum lte_lc_modem_evt_type {
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
	 * Selected CE level in RACH procedure. See 3GPP TS 36.331 for details.
	 *
	 * The associated payload is the @c lte_lc_modem_evt.ce_level member of type
	 * @ref lte_lc_ce_level in the event.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf91x1
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_MODEM_EVT_CE_LEVEL,

	/**
	 * RF calibration or power class selection not done.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf91x1
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_MODEM_EVT_RF_CAL_NOT_DONE,

	/**
	 * Invalid band configuration.
	 *
	 * The associated payload is the @c lte_lc_modem_evt.invalid_band_conf member of type
	 * @ref lte_lc_invalid_band_conf in the event.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf91x1
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_MODEM_EVT_INVALID_BAND_CONF,

	/**
	 * Current country the device is operating in.
	 *
	 * The associated payload is the @c lte_lc_modem_evt.detected_country member of type
	 * `uint32_t` in the event. The country is reported as a Mobile Country Code (MCC).
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_MODEM_EVT_DETECTED_COUNTRY
};

/** @brief Band configuration status. */
enum lte_lc_band_conf_status {
	/** Usable bands available in system. */
	LTE_LC_BAND_CONF_STATUS_OK = 0,

	/** No usable bands available in system. */
	LTE_LC_BAND_CONF_STATUS_INVALID = 1,

	/** System support not configured. */
	LTE_LC_BAND_CONF_STATUS_SYSTEM_NOT_SUPPORTED = 2
};

/** @brief Detected conflicting band lock or operator restrictions. */
struct lte_lc_invalid_band_conf {
	/** @brief LTE-M band configuration status. */
	enum lte_lc_band_conf_status status_ltem;

	/** @brief NB-IoT band configuration status. */
	enum lte_lc_band_conf_status status_nbiot;

	/** @brief NTN NB-IoT band configuration status. */
	enum lte_lc_band_conf_status status_ntn_nbiot;
};

/** @brief Modem domain event.
 *
 * This structure is used as the payload for event @ref LTE_LC_EVT_MODEM_EVENT.
 */
struct lte_lc_modem_evt {
	/** @brief Event type. */
	enum lte_lc_modem_evt_type type;

	/** @brief Payload for the event (optional). */
	union {
		/** @brief Payload for event @ref LTE_LC_MODEM_EVT_CE_LEVEL. */
		enum lte_lc_ce_level ce_level;

		/** @brief Payload for event @ref LTE_LC_MODEM_EVT_INVALID_BAND_CONF. */
		struct lte_lc_invalid_band_conf invalid_band_conf;

		/**
		 * @brief Payload for event @ref LTE_LC_MODEM_EVT_DETECTED_COUNTRY.
		 *
		 * Mobile Country Code (MCC) for the detected country.
		 */
		uint32_t detected_country;
	};
};

/** @brief RAI configuration. */
struct lte_lc_rai_cfg {
	/** @brief E-UTRAN cell ID. */
	uint32_t cell_id;
	/** @brief Mobile Country Code. */
	int mcc;
	/** @brief Mobile Network Code. */
	int mnc;
	/** @brief AS RAI support. */
	bool as_rai;
	/** @brief CP RAI support. */
	bool cp_rai;
};

/**
 * @brief Connection evaluation parameters.
 *
 * @note For more information on the various connection evaluation parameters, refer to the
 *       "nRF91 AT Commands - Command Reference Guide".
 */
struct lte_lc_conn_eval_params {
	/** @brief RRC connection state during measurements. */
	enum lte_lc_rrc_mode rrc_state;

	/**
	 * @brief Relative estimated energy consumption for data transmission compared to nominal
	 * consumption.
	 */
	enum lte_lc_energy_estimate energy_estimate;

	/**
	 * @brief Value that indicates if the evaluated cell is a part of the
	 * Tracking Area Identifier list received from the network.
	 */
	enum lte_lc_tau_triggered tau_trig;

	/**
	 * @brief Coverage Enhancement level for PRACH.
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

	/** @brief EARFCN for given cell where EARFCN is per 3GPP TS 36.101. */
	int earfcn;

	/** @brief Reduction in power density in dB. */
	int16_t dl_pathloss;

	/**
	 * @brief Current RSRP level at time of report.
	 *
	 * Can be converted into dBm using @ref RSRP_IDX_TO_DBM macro.
	 *
	 * * -17: RSRP < -156 dBm
	 * * -16: -156 ≤ RSRP < -155 dBm
	 * * ...
	 * * -3: -143 ≤ RSRP < -142 dBm
	 * * -2: -142 ≤ RSRP < -141 dBm
	 * * -1: -141 ≤ RSRP < -140 dBm
	 * * 0: Not used.
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
	 * @brief Current RSRQ level at time of report.
	 *
	 * Can be converted into dB using @ref RSRQ_IDX_TO_DB macro.
	 *
	 * * -30: RSRQ < -34 dB
	 * * -29: -34 ≤ RSRQ < -33.5 dB
	 * * ...
	 * * -2: -20.5 ≤ RSRQ < -20 dB
	 * * -1: -20 ≤ RSRQ < -19.5 dB
	 * * 0: Not used.
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
	 * @brief Estimated TX repetitions.
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
	 * @brief Estimated RX repetitions.
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

	/** @brief Physical cell ID of evaluated cell. */
	int16_t phy_cid;

	/**
	 * @brief Current band information.
	 *
	 * 0 when band information is not available.
	 *
	 * See 3GPP TS 36.101 for details.
	 */
	int16_t band;

	/**
	 * @brief Current signal-to-noise ratio at time of report.
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
	 * @brief Estimated TX power in dBm.
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

	/** @brief Mobile Country Code. */
	int mcc;

	/** @brief Mobile Network Code. */
	int mnc;

	/** @brief E-UTRAN cell ID. */
	uint32_t cell_id;
};

/**
 * @brief Specifies which type of search the modem should perform when a
 * neighbor cell measurement is started.
 *
 * When using search types up to @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE, the result
 * contains parameters from current/serving cell and optionally up to
 * `CONFIG_LTE_NEIGHBOR_CELLS_MAX` neighbor cells.
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
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT = 2,

	/**
	 * The modem follows the same procedure as for
	 * @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT, but will continue to perform a complete
	 * search instead of a light search, and the search is performed for all supported bands.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE = 3,

	/**
	 * GCI search, option 1. Modem searches EARFCNs based on previous cell history.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT = 4,

	/**
	 * GCI search, option 2. Modem starts with the same search method as in
	 * @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT. If less than gci_count cells were found,
	 * the modem performs a light search on bands that are valid for the area of the current
	 * ITU-T region.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT = 5,

	/**
	 * GCI search, option 3. Modem starts with the same search method as in
	 * @ref LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT. If less than gci_count cells were found,
	 * the modem performs a complete search on all supported bands.
	 */
	LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE = 6,
};

/** @brief Neighbor cell measurement initiation parameters. */
struct lte_lc_ncellmeas_params {
	/** @brief Search type, @ref lte_lc_neighbor_search_type. */
	enum lte_lc_neighbor_search_type search_type;

	/**
	 * @brief Maximum number of GCI cells to be searched. Integer, range: 2-15.
	 *
	 * Current cell is counted as one cell. Mandatory with the GCI search types, ignored with
	 * other search types.
	 */
	uint8_t gci_count;
};

/** @brief Environment evaluation type. */
enum lte_lc_env_eval_type {
	/**
	 * PLMN search is stopped after light search if any of the PLMNs to evaluate were found.
	 * Search is continued over all frequency bands if light search did not find any results.
	 */
	LTE_LC_ENV_EVAL_TYPE_DYNAMIC = 0,

	/**
	 * PLMN search is stopped after light search even if no PLMNs to evaluate
	 * were found.
	 */
	LTE_LC_ENV_EVAL_TYPE_LIGHT = 1,

	/** PLMN search covers all channels in all supported frequency bands. */
	LTE_LC_ENV_EVAL_TYPE_FULL = 2
};

/** @brief PLMN to evaluate. */
struct lte_lc_env_eval_plmn {
	/** @brief Mobile Country Code (MCC). */
	int mcc;

	/** @brief Mobile Network Code (MNC). */
	int mnc;
};

/** @brief Environment evaluation parameters. */
struct lte_lc_env_eval_params {
	/**
	 * @brief Environment evaluation type.
	 */
	enum lte_lc_env_eval_type eval_type;

	/**
	 * @brief Number of PLMNs to evaluate.
	 *
	 * Must be less than or equal to @c CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT.
	 */
	uint8_t plmn_count;

	/**
	 * @brief Pointer to an array of PLMNs to evaluate.
	 */
	struct lte_lc_env_eval_plmn *plmn_list;
};

/** @brief Search pattern type. */
enum lte_lc_periodic_search_pattern_type {
	/** Range search pattern. */
	LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE = 0,

	/** Table search pattern. */
	LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE = 1,
};

/** @brief Configuration for periodic search of type @ref LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE. */
struct lte_lc_periodic_search_range_cfg {
	/**
	 * @brief Sleep time between searches in the beginning of the range.
	 *
	 * Allowed values: 0 - 65535 seconds.
	 */
	uint16_t initial_sleep;

	/**
	 * @brief Sleep time between searches in the end of the range.
	 *
	 * Allowed values: 0 - 65535 seconds.
	 */
	uint16_t final_sleep;

	/**
	 * @brief Optional target time in minutes for achieving the @c final_sleep value.
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
	 * @brief Time that must elapse before entering the next search pattern.
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
 * @brief Configuration for periodic search of type @ref LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE.
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
	/** @brief Mandatory sleep time. */
	int val_1;

	/**
	 * @brief Optional sleep time.
	 *
	 * Must be set to -1 if not used.
	 */
	int val_2;

	/**
	 * @brief Optional sleep time.
	 *
	 * @c val_2 must be configured for this parameter to take effect.
	 *
	 *  Must be set to -1 if not used.
	 */
	int val_3;

	/**
	 * @brief Optional sleep time.
	 *
	 * @c val_3 must be configured for this parameter to take effect.
	 *
	 * Must be set to -1 if not used.
	 */
	int val_4;

	/**
	 * @brief Optional sleep time.
	 *
	 * @c val_4 must be configured for this parameter to take effect.
	 *
	 * Must be set to -1 if not used.
	 */
	int val_5;
};

/**
 * @brief Periodic search pattern.
 *
 * A search pattern may be of either 'range' or 'table' type.
 */
struct lte_lc_periodic_search_pattern {
	/** @brief Search pattern type. */
	enum lte_lc_periodic_search_pattern_type type;

	union {
		/** @brief Configuration for periodic search of type 'range'. */
		struct lte_lc_periodic_search_range_cfg range;

		/** @brief Configuration for periodic search of type 'table'. */
		struct lte_lc_periodic_search_table_cfg table;
	};
};

/** @brief Periodic search configuration. */
struct lte_lc_periodic_search_cfg {
	/**
	 * @brief Indicates if the last given pattern is looped from the beginning
	 * when the pattern has ended.
	 *
	 * If several patterns are configured, this impacts only the last pattern.
	 */
	bool loop;

	/**
	 * @brief Indicates if the modem can return to a given search pattern with
	 * shorter sleeps, for example, when radio conditions change and the given
	 * pattern index has already been exceeded.
	 *
	 * Allowed values:
	 * * 0: No return pattern.
	 * * 1 - 4: Return to search pattern index 1..4.
	 */
	uint16_t return_to_pattern;

	/**
	 * @brief Indicates if band optimization shall be used.
	 *
	 * * 0: No optimization. Every periodic search shall be all band search.
	 * * 1: Use default optimizations predefined by modem. Predefinition depends on
	 *      the active data profile.
	 *      See "nRF91 AT Commands - Command Reference Guide" for more information.
	 * * 2 - 20: Every n periodic search must be an all band search.
	 */
	uint16_t band_optimization;

	/** @brief The number of valid patterns. Range 1 - 4. */
	size_t pattern_count;

	/** @brief Array of periodic search patterns. */
	struct lte_lc_periodic_search_pattern patterns[4];
};

#if defined(CONFIG_LTE_LC_ENV_EVAL_MODULE)
/**
 * @brief Environment evaluation results.
 *
 * This structure is used as the payload for event @ref LTE_LC_EVT_ENV_EVAL_RESULT.
 */
struct lte_lc_env_eval_result {
	/**
	 * @brief Status for the environment evaluation.
	 *
	 * 0 indicates successful completion of the evaluation.
	 * 5 indicates that evaluation failed, aborted due to higher priority operation.
	 * 7 indicates that evaluation failed, unspecified.
	 */
	uint8_t status;

	/**
	 * @brief Number of PLMN results available in the results array.
	 *
	 * Range: 0 to CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT
	 */
	uint8_t result_count;

	/**
	 * @brief Pointer to an array of environment evaluation results for different PLMNs.
	 *
	 * Each entry contains the evaluation result for a specific PLMN.
	 */
	struct lte_lc_conn_eval_params *results;
};
#endif /* CONFIG_LTE_LC_ENV_EVAL_MODULE */

/** @brief Callback for modem functional mode changes. */
struct lte_lc_cfun_cb {
	void (*callback)(enum lte_lc_func_mode, void *ctx);
	void *context;
};

enum lte_lc_pdn_evt_type {
	/** PDN connection activated.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_ACTIVATED,

	/**
	 * PDN connection deactivated.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_DEACTIVATED,

	/**
	 * PDN has IPv6 connectivity.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_IPV6_UP,

	/**
	 * PDN has lost IPv6 connectivity.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_IPV6_DOWN,

	/**
	 * PDN is suspended.
	 *
	 * PDNs can be suspended when cellular profiles are used. While suspended, the PDNs remain
	 * active, but cannot be used for data transmission. PDNs associated with a cellular
	 * profile are suspended when the device is switched to flight mode using
	 * @ref LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG or @ref LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG_UICC_ON.
	 * The PDNs remain suspended when switching to a different access technology. When the
	 * device is switched back to the original access technology and LTE is activated, the
	 * associated PDNs are resumed.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_EVT_PDN_SUSPENDED,

	/**
	 * PDN is resumed.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 *
	 * @note This is only supported by the following modem firmware:
	 *       - mfw_nrf9151-ntn
	 */
	LTE_LC_EVT_PDN_RESUMED,

	/**
	 * Network detached.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_NETWORK_DETACH,

	/**
	 * APN rate control is ON for the given PDN.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON,

	/**
	 * APN rate control is OFF for the given PDN.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF,

	/**
	 * PDP context is destroyed for the given PDN.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event.
	 */
	LTE_LC_EVT_PDN_CTX_DESTROYED,

	/**
	 * ESM error notification for PDN.
	 *
	 * The associated payload is the @c lte_lc_evt.pdn member of type
	 * @ref lte_lc_pdn_evt in the event. The @c lte_lc_pdn_evt.esm_err member contains
	 * the ESM error code.
	 */
	LTE_LC_EVT_PDN_ESM_ERROR,
};

/** @brief PDN event payload. */
struct lte_lc_pdn_evt {
	/** @brief Event type. */
	enum lte_lc_pdn_evt_type type;
	/** @brief PDP context ID. */
	uint8_t cid;
	/** @brief ESM error code. Only valid for @ref LTE_LC_EVT_PDN_ESM_ERROR event. */
	int esm_err;
	/**
	 * @brief Cellular profile ID.
	 *
	 * Only valid for @ref LTE_LC_EVT_PDN_NETWORK_DETACH event when a
	 * cellular profile ID is present in the notification from the modem.
	 * The value is set to @c -1 when no cellular profile ID was provided.
	 */
	int8_t cellular_profile_id;
};

struct lte_lc_cellular_profile_evt {
	/** @brief Cellular profile ID. */
	int8_t profile_id;
};

/** @brief LTE event. */
struct lte_lc_evt {
	/** @brief Event type. */
	enum lte_lc_evt_type type;

	union {
		/** @brief Payload for event @ref LTE_LC_EVT_NW_REG_STATUS. */
		enum lte_lc_nw_reg_status nw_reg_status;

		/** @brief Payload for event @ref LTE_LC_EVT_RRC_UPDATE. */
		enum lte_lc_rrc_mode rrc_mode;

#if defined(CONFIG_LTE_LC_PSM_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_PSM_UPDATE. */
		struct lte_lc_psm_cfg psm_cfg;
#endif /* CONFIG_LTE_LC_PSM_MODULE */

#if defined(CONFIG_LTE_LC_EDRX_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_EDRX_UPDATE. */
		struct lte_lc_edrx_cfg edrx_cfg;
#endif /* CONFIG_LTE_LC_EDRX_MODULE */

		/** @brief Payload for event @ref LTE_LC_EVT_CELL_UPDATE. */
		struct lte_lc_cell cell;

		/** @brief Payload for event @ref LTE_LC_EVT_LTE_MODE_UPDATE. */
		enum lte_lc_lte_mode lte_mode;

#if defined(CONFIG_LTE_LC_MODEM_SLEEP_MODULE)
		/**
		 * @brief Payload for events @ref LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING,
		 * @ref LTE_LC_EVT_MODEM_SLEEP_EXIT and @ref LTE_LC_EVT_MODEM_SLEEP_ENTER.
		 */
		struct lte_lc_modem_sleep modem_sleep;
#endif /* CONFIG_LTE_LC_MODEM_SLEEP_MODULE */

		/** @brief Payload for event @ref LTE_LC_EVT_MODEM_EVENT. */
		struct lte_lc_modem_evt modem_evt;

		/**
		 * @brief Payload for event @ref LTE_LC_EVT_TAU_PRE_WARNING.
		 *
		 * Time until next Tracking Area Update in milliseconds.
		 */
		uint64_t time;

#if defined(CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_NEIGHBOR_CELL_MEAS. */
		struct lte_lc_cells_info cells_info;
#endif /* CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE */

#if defined(CONFIG_LTE_LC_RAI_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_RAI_UPDATE. */
		struct lte_lc_rai_cfg rai_cfg;
#endif /* CONFIG_LTE_LC_RAI_MODULE */

#if defined(CONFIG_LTE_LC_ENV_EVAL_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_ENV_EVAL_RESULT. */
		struct lte_lc_env_eval_result env_eval_result;
#endif /* CONFIG_LTE_LC_ENV_EVAL_MODULE */

#if defined(CONFIG_LTE_LC_PDN_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_PDN. */
		struct lte_lc_pdn_evt pdn;
#endif /* CONFIG_LTE_LC_PDN_MODULE */

#if defined(CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE)
		/** @brief Payload for event @ref LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE. */
		struct lte_lc_cellular_profile_evt cellular_profile;
#endif /* CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE */
	};
};

/** @brief PDN address family. */
enum lte_lc_pdn_family {
	/** IPv4 */
	LTE_LC_PDN_FAM_IPV4   = 0,
	/** IPv6 */
	LTE_LC_PDN_FAM_IPV6   = 1,
	/** IPv4 and IPv6 */
	LTE_LC_PDN_FAM_IPV4V6 = 2,
	/** Non-IP */
	LTE_LC_PDN_FAM_NONIP  = 3,
};

/** @brief Additional Packet Data Protocol (PDP) context configuration options. */
struct lte_lc_pdn_pdp_context_opts {
	/**
	 * @brief IPv4 address allocation.
	 *
	 * - 0: IPv4 address through Non-access Stratum (NAS) signaling (default)
	 * - 1: IPv4 address through Dynamic Host Configuration Protocol (DHCP)
	 */
	uint8_t ip4_addr_alloc;
	/**
	 * @brief NAS Signalling Low Priority Indication.
	 *
	 * - 0: Use Non-access Stratum (NAS) Signalling Low Priority Indication (NSLPI) value
	 *      from configuration (default)
	 * - 1: Use value "Not configured" for NAS Signalling Low Priority Indication
	 */
	uint8_t nslpi;
	/**
	 * @brief Protected transmission of Protocol Configuration Options (PCO).
	 *
	 * - 0: Protected transmission of PCO is not requested (default)
	 * - 1: Protected transmission of PCO is requested
	 */
	uint8_t secure_pco;
};

/**
 * @brief PDN connection dynamic information structure.
 *
 * This structure holds dynamic information about the PDN connection.
 */
struct lte_lc_pdn_dynamic_info {
	/** @brief IPv4 Maximum Transmission Unit. */
	uint32_t ipv4_mtu;
	/** @brief IPv6 Maximum Transmission Unit. */
	uint32_t ipv6_mtu;
	/** @brief Primary IPv4 DNS address. */
	struct net_in_addr dns_addr4_primary;
	/** @brief Secondary IPv4 DNS address. */
	struct net_in_addr dns_addr4_secondary;
	/** @brief Primary IPv6 DNS address. */
	struct net_in6_addr dns_addr6_primary;
	/** @brief Secondary IPv6 DNS address. */
	struct net_in6_addr dns_addr6_secondary;
};

/** @brief Authentication method. */
enum lte_lc_pdn_auth {
	/** No authentication. */
	LTE_LC_PDN_AUTH_NONE = 0,
	/** Password Authentication Protocol (PAP). */
	LTE_LC_PDN_AUTH_PAP  = 1,
	/** Challenge-Handshake Authentication Protocol (CHAP). */
	LTE_LC_PDN_AUTH_CHAP = 2,
};

/** @brief UICC configuration type, used for cellular profile selection */
enum lte_lc_uicc {
	/** Physical SIM or eSIM */
	LTE_LC_UICC_PHYSICAL	= 0,
	/** SoftSIM */
	LTE_LC_UICC_SOFTSIM	= 2,
};

/**
 * @brief LTE access technology (AcT) bitmap.
 *
 * This bitmap selects which access technologies are enabled for an operation.
 * It maps directly to the AcT bitmask in AT commands such as `AT%CELLULARPRFL`
 * and `AT%PALL`.
 */
enum lte_lc_act {
	/** LTE-M (Cat-M1). */
	LTE_LC_ACT_LTEM		= BIT(0),
	/** NB-IoT. */
	LTE_LC_ACT_NBIOT	= BIT(1),
	/** NTN NB-IoT. */
	LTE_LC_ACT_NTN		= BIT(2),
};

/**
 * @brief Cellular profile.
 *
 * Cellular profiles are used when there is a need for simultaneous terrestrial and
 * non-terrestrial LTE registrations and PDN connections. There can be only one physical
 * access at a time, but a second access is logically stored by the modem. Typically,
 * this means that there are more than one USIM card or profile, so that certain USIM
 * card or profile is used in certain access technology. A cellular profile combines
 * the information that is needed for LTE registration and is identified by the
 * cellular profile ID.
 *
 * @note The profile ID 0 is associated with PDP contexts in the range 0-9, while profile ID 1
 *	 is associated with PDP contexts in the range 10-19. The default context for each profile
 *	 is the first context in the range.
 */
struct lte_lc_cellular_profile {
	/** @brief Cellular profile ID. Valid values are 0 and 1. */
	uint8_t id;

	/**
	 * @brief Access technology bitmap for the profile.
	 *
	 * This is a combination of @ref lte_lc_act values.
	 */
	uint8_t act;

	/** @brief UICC configuration */
	enum lte_lc_uicc uicc;
};


/**
 * @brief Handler for LTE events.
 *
 * @param[in] evt Event.
 */
typedef void(*lte_lc_evt_handler_t)(const struct lte_lc_evt *const evt);

/**
 * @brief Register handler for LTE events.
 *
 * @param[in] handler Event handler.
 */
void lte_lc_register_handler(lte_lc_evt_handler_t handler);

/**
 * @brief De-register handler for LTE events.
 *
 * @param[in] handler Event handler.
 *
 * @retval 0 if successful.
 * @retval -ENXIO if the handler was not found.
 * @retval -EINVAL if the handler was a @c NULL pointer.
 */
int lte_lc_deregister_handler(lte_lc_evt_handler_t handler);

/**
 * @brief Connect to LTE network.
 *
 * This function sets the modem to online mode using @ref lte_lc_normal.
 * The client does not need to do anything to re-connect if the connection is lost but
 * the modem will handle it automatically.
 *
 * @note After initialization, the system mode and LTE preference are set to the
 * default modes selected with Kconfig.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
 * @retval -ETIMEDOUT if a connection attempt timed out before the device was
 *	   registered to a network.
 * @retval -EINPROGRESS if a connection establishment attempt is already in progress.
 */
int lte_lc_connect(void);

/**
 * @brief Connect to LTE network.
 *
 * The function returns immediately.
 *
 * This function sets the modem to online mode using @ref lte_lc_normal.
 * The client does not need to do anything to re-connect if the connection is lost but
 * the modem will handle it automatically.
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
 * @brief Set the modem to offline mode.
 *
 * Disables both transmit and receive RF circuits and deactivates LTE
 * and GNSS services.
 *
 * @note Requires `CONFIG_LTE_LC_FUNCTIONAL_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_offline(void);

/**
 * @brief Set the modem to power off mode.
 *
 * @note Requires `CONFIG_LTE_LC_FUNCTIONAL_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_power_off(void);

/**
 * @brief Set the modem to normal mode.
 *
 * @note Requires `CONFIG_LTE_LC_FUNCTIONAL_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the functional mode could not be configured.
 */
int lte_lc_normal(void);

/**
 * @brief Set modem PSM parameters.
 *
 * Requested periodic TAU (RPTAU) and requested active time (RAT) are used when PSM mode is
 * subsequently enabled using lte_lc_psm_req(). These are the requested values and network
 * determines the final values.
 *
 * For encoding of the variables, see nRF AT Commands Reference Guide, 3GPP 27.007 Ch. 7.38., and
 * 3GPP 24.008 Ch. 10.5.7.4a and Ch. 10.5.7.3.
 *
 * @note Requires `CONFIG_LTE_LC_PSM_MODULE` to be enabled.
 *
 * @param[in] rptau
 * @parblock
 *          Requested periodic TAU as a null-terminated 8 character long bit field string.
 *          Set to @c NULL to use modem's default value.
 *          See 3GPP 24.008 Ch. 10.5.7.4a for data format.
 *
 *          For example, value of 32400 s is represented as '00101001'.
 *
 *          Bits 5 to 1 represent the binary coded timer value that is multiplied by timer unit.
 *
 *          Bits 8 to 6 define the timer unit as follows:
 *
 *          - 000: 10 minutes
 *          - 001: 1 hour
 *          - 010: 10 hours
 *          - 011: 2 seconds
 *          - 100: 30 seconds
 *          - 101: 1 minute
 *          - 110: 320 hours
 * @endparblock
 *
 * @param[in] rat
 * @parblock
 *          Requested active time as a null-terminated string.
 *          Set to @c NULL to use modem's default value.
 *          See 3GPP 24.008 Ch. 10.5.7.3 for data format.
 *
 *          For example, value of 120 s is represented as '00100010'.
 *
 *          Bits 5 to 1 represent the binary coded timer value that is multiplied by timer unit.
 *
 *          Bits 8 to 6 define the timer unit as follows:
 *
 *          - 000: 2 seconds
 *          - 001: 1 minute
 *          - 010: 6 minutes
 *          - 111: Timer is deactivated
 * @endparblock
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_psm_param_set(const char *rptau, const char *rat);

/**
 * @brief Set modem PSM parameters.
 *
 * Requested periodic TAU (RPTAU) and requested active time (RAT) are used when PSM mode is
 * subsequently enabled using lte_lc_psm_req(). These are the requested values and network
 * determines the final values.
 *
 * This is a convenience function to set PSM parameters in seconds while lte_lc_psm_param_set()
 * requires the caller to encode the values. However, note that 3GPP specifications do
 * not support all possible integer values but the encoding limits the potential values.
 * The values are rounded up to the next possible value. There may be significant
 * rounding up, especially when rptau is tens and hundreds of hours.
 *
 * For more information about the encodings, see the description of the
 * lte_lc_psm_param_set() function.
 *
 * @note Requires `CONFIG_LTE_LC_PSM_MODULE` to be enabled.
 *
 * @param[in] rptau
 * @parblock
 *          Requested periodic TAU in seconds as a non-negative integer. Range 0 - 35712000 s.
 *          Set to @c -1 to use modem's default value.
 *          The given value will be rounded up to the closest possible value that is
 *          calculated by multiplying the timer unit with the timer value:
 *
 *          - Timer unit is one of: 2 s, 30 s, 60 s, 600 s (10 min), 3600 s (1 h), 36000 s (10 h),
 *            1152000 s (320 h)
 *          - Timer value range is 0-31
 * @endparblock
 *
 * @param[in] rat
 * @parblock
 *          Requested active time in seconds as a non-negative integer. Range 0 - 11160s.
 *          Set to @c -1 to use modem's default value.
 *          The given value will be rounded up to the closest possible value that is
 *          calculated by multiplying the timer unit with the timer value:
 *
 *          - Timer unit is one of: 2 s, 30 s, 360 s (6 min)
 *          - Timer value range is 0-31
 * @endparblock
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_psm_param_set_seconds(int rptau, int rat);

/**
 * @brief Request modem to enable or disable Power Saving Mode (PSM).
 *
 * PSM parameters can be set using `CONFIG_LTE_PSM_REQ_FORMAT`,
 * `CONFIG_LTE_PSM_REQ_RPTAU`, `CONFIG_LTE_PSM_REQ_RAT`,
 * `CONFIG_LTE_PSM_REQ_RPTAU_SECONDS` and `CONFIG_LTE_PSM_REQ_RAT_SECONDS`,
 * or by calling lte_lc_psm_param_set() or lte_lc_psm_param_set_seconds().
 *
 * @note `CONFIG_LTE_PSM_REQ` can be set to enable PSM, which is generally sufficient. This
 *       option allows explicit disabling/enabling of PSM requesting after modem initialization.
 *       Calling this function for run-time control is possible, but it should be noted that
 *       conflicts may arise with the value set by `CONFIG_LTE_PSM_REQ` if it is called
 *       during modem initialization.
 *
 * @note Requires `CONFIG_LTE_LC_PSM_MODULE` to be enabled.
 *
 * @param[in] enable @c true to enable PSM, @c false to disable PSM.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_psm_req(bool enable);

/**
 * @brief Get the current PSM (Power Saving Mode) configuration.
 *
 * @note Requires `CONFIG_LTE_LC_PSM_MODULE` to be enabled.
 *
 * @param[out] tau Periodic TAU interval in seconds. A non-negative integer.
 * @param[out] active_time Active time in seconds. A non-negative integer,
 *                         or @c -1 if PSM is deactivated.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EBADMSG if no active time and/or TAU value was received, including the case when
 *         modem is not registered to network.
 */
int lte_lc_psm_get(int *tau, int *active_time);

/**
 * @brief Request modem to enable or disable
 * [proprietary Power Saving Mode](https://docs.nordicsemi.com/bundle/ref_at_commands_nrf91x1/page/REF/at_commands/nw_service/feaconf_set.html).
 *
 * The purpose of the proprietary PSM feature is to perform a PSM-like sleep when network does not
 * allow normal PSM usage. During proprietary PSM, modem will fall to sleep in the same way than it
 * would do if network allowed to use PSM. Sending of MO data or MO SMS will automatically wake up
 * the modem just like if modem was in normal PSM sleep.
 *
 * To use the feature, also PSM request must be enabled using `CONFIG_LTE_PSM_REQ` or
 * lte_lc_psm_req().
 *
 * Refer to the AT command guide for guidance and limitations of this feature.
 *
 * @note `CONFIG_LTE_PROPRIETARY_PSM_REQ` can be set to enable proprietary PSM, which is
 *       generally sufficient. This option allows to enable or disable proprietary PSM after modem
 *       initialization. Calling this function for run-time control is possible, but it should be
 *       noted that conflicts may arise with the value set by
 *       `CONFIG_LTE_PROPRIETARY_PSM_REQ` if it is called during modem initialization.
 *
 * @note This is only supported by the following modem firmware:
 *       - mfw_nrf91x1
 *       - mfw_nrf9151-ntn
 *
 * @note Requires `CONFIG_LTE_LC_PSM_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_proprietary_psm_req(bool enable);

/**
 * @brief Set the Paging Time Window (PTW) value to be used with eDRX.
 *
 * eDRX can be requested using lte_lc_edrx_req(). PTW is set individually for LTE-M, NB-IoT and
 * NTN NB-IoT.
 *
 * Requesting a specific PTW configuration should be done with caution. The requested value must be
 * compliant with the eDRX value that is configured, and it is usually best to let the modem
 * use default PTW values.
 *
 * For reference to which values can be set, see subclause 10.5.5.32 of 3GPP TS 24.008.
 *
 * @note Requires `CONFIG_LTE_LC_EDRX_MODULE` to be enabled.
 *
 * @param[in] mode LTE mode to which the PTW value applies.
 * @param[in] ptw Paging Time Window value as a null-terminated string.
 *                Set to @c NULL to use modem's default value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_ptw_set(enum lte_lc_lte_mode mode, const char *ptw);

/**
 * @brief Set the requested eDRX value.
 *
 * eDRX can be requested using lte_lc_edrx_req(). eDRX value is set individually for LTE-M,
 * NB-IoT and NTN NB-IoT.
 *
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @note Requires `CONFIG_LTE_LC_EDRX_MODULE` to be enabled.
 *
 * @param[in] mode LTE mode to which the eDRX value applies.
 * @param[in] edrx eDRX value as a null-terminated string.
 *                 Set to @c NULL to use modem's default value.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an input parameter was invalid.
 */
int lte_lc_edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx);

/**
 * @brief Request modem to enable or disable use of eDRX.
 *
 * eDRX parameters can be set using `CONFIG_LTE_EDRX_REQ_VALUE_LTE_M`,
 * `CONFIG_LTE_EDRX_REQ_VALUE_NBIOT`, `CONFIG_LTE_PTW_VALUE_LTE_M` and
 * `CONFIG_LTE_PTW_VALUE_NBIOT`, or by calling lte_lc_edrx_param_set() and
 * lte_lc_ptw_set().
 *
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @note `CONFIG_LTE_EDRX_REQ` can be set to enable eDRX, which is generally sufficient.
 *       This option allows explicit disabling/enabling of eDRX requesting after modem
 *       initialization. Calling this function for run-time control is possible, but it should be
 *       noted that conflicts may arise with the value set by `CONFIG_LTE_EDRX_REQ` if it is
 *       called during modem initialization.
 *
 * @note Requires `CONFIG_LTE_LC_EDRX_MODULE` to be enabled.
 *
 * @param[in] enable @c true to enable eDRX, @c false to disable eDRX.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_edrx_req(bool enable);

/**
 * @brief Get the eDRX parameters currently provided by the network.
 *
 * @param[out] edrx_cfg eDRX configuration.
 *
 * @note Requires `CONFIG_LTE_LC_EDRX_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 * @retval -EBADMSG if parsing of the AT command response failed.
 */
int lte_lc_edrx_get(struct lte_lc_edrx_cfg *edrx_cfg);

/**
 * @brief Get the current network registration status.
 *
 * @param[out] status Network registration status.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the network registration could not be retrieved from the modem.
 */
int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status);

/**
 * @brief Set the modem's system mode and LTE preference.
 *
 * @param[in] mode System mode.
 * @param[in] preference System mode preference.
 *
 * @note Requires `CONFIG_LTE_LC_SYSTEM_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the network registration could not be retrieved from the modem.
 */
int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference);

/**
 * @brief Get the modem's system mode and LTE preference.
 *
 * @param[out] mode System mode.
 * @param[out] preference System mode preference. Can be @c NULL.
 *
 * @note Requires `CONFIG_LTE_LC_SYSTEM_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the system mode could not be retrieved from the modem.
 */
int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference);

/**
 * @brief Set the modem's functional mode.
 *
 * @param[in] mode Functional mode.
 *
 * @note Requires `CONFIG_LTE_LC_FUNCTIONAL_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the functional mode could not be retrieved from the modem.
 */
int lte_lc_func_mode_set(enum lte_lc_func_mode mode);

/**
 * @brief Get the modem's functional mode.
 *
 * @param[out] mode Functional mode.
 *
 * @note Requires `CONFIG_LTE_LC_FUNCTIONAL_MODE_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the functional mode could not be retrieved from the modem.
 */
int lte_lc_func_mode_get(enum lte_lc_func_mode *mode);

/**
 * @brief Get the currently active LTE mode.
 *
 * @param[out] mode LTE mode.
 *
 * @note Requires `CONFIG_LTE_LC_CONNECTION_STATUS_MODULE` to be enabled.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if the current LTE mode could not be retrieved.
 * @retval -EBADMSG if the LTE mode was not recognized.
 */
int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode);

/**
 * @brief Initiate a neighbor cell measurement.
 *
 * The result of the measurement is reported back as an event of the type
 * @ref LTE_LC_EVT_NEIGHBOR_CELL_MEAS, meaning that an event handler must be registered to receive
 * the information. Depending on the network conditions, LTE connection state and requested search
 * type, it may take a while before the measurement result is ready and reported back. After the
 * event is received, the neighbor cell measurements are automatically stopped. If the
 * function returns successfully, the @ref LTE_LC_EVT_NEIGHBOR_CELL_MEAS event is always reported.
 *
 * In receive only functional mode, it is recommended to wait for the modem to complete the network
 * selection before calling this function. This can be determined from the
 * @ref LTE_LC_EVT_MODEM_EVENT event with modem event @ref LTE_LC_MODEM_EVT_SEARCH_DONE.
 *
 * @note In @ref LTE_LC_FUNC_MODE_RX_ONLY functional mode, this is only supported by the following
 *       modem firmware:
 *       - mfw_nrf91x1 v2.0.3 or later
 *       - mfw_nrf9151-ntn
 *
 * @note Requires `CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE` to be enabled.
 *
 * @param[in] params Search type parameters or @c NULL to initiate a measurement with the default
 *                   parameters. See @ref lte_lc_ncellmeas_params for more information.
 *
 * @retval 0 if neighbor cell measurement was successfully initiated.
 * @retval -EFAULT if AT command failed.
 * @retval -EINVAL if parameters are invalid.
 * @retval -EINPROGRESS if a neighbor cell measurement is already in progress.
 */
int lte_lc_neighbor_cell_measurement(struct lte_lc_ncellmeas_params *params);

/**
 * @brief Cancel an ongoing neighbor cell measurement.
 *
 * @note Requires `CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE` to be enabled.
 *
 * @retval 0 if neighbor cell measurement was cancelled.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_neighbor_cell_measurement_cancel(void);

/**
 * @brief Get connection evaluation parameters.
 *
 * Connection evaluation parameters can be used to determine the energy efficiency of data
 * transmission before the actual data transmission. Connection evaluation is based on collected
 * cell history.
 *
 * In receive only functional mode with modem firmware mfw_nrf91x1 v2.0.3 and higher, and
 * mfw_nrf9151-ntn, it is recommended to wait for the modem to complete the network selection
 * before calling this function. This can be determined from the @ref LTE_LC_EVT_MODEM_EVENT event
 * with modem event @ref LTE_LC_MODEM_EVT_SEARCH_DONE.
 *
 * @note Requires `CONFIG_LTE_LC_CONN_EVAL_MODULE` to be enabled.
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
 * @brief Start environment evaluation.
 *
 * Perform evaluation for PLMN selection. Evaluates available PLMNs and provides information
 * of their estimated signalling conditions. Based on the evaluation results, the application
 * can then select the best PLMN to use. This is useful especially in cases where the device
 * has multiple SIMs or SIM profiles to select from.
 *
 * PLMNs (MCC/MNC pairs) to be evaluated are listed in the @ref lte_lc_env_eval_params
 * structure. For each PLMN, evaluation results for the best found cell are returned. The results
 * are returned with the @ref LTE_LC_EVT_ENV_EVAL_RESULT event.
 *
 * Environment evaluation can only be performed in receive only functional mode. The device does
 * not transmit anything during the evaluation.
 *
 * @note This is only supported by the following modem firmware:
 *       - mfw_nrf91x1 v2.0.3 or later
 *       - mfw_nrf9151-ntn
 *
 * @note Requires `CONFIG_LTE_LC_ENV_EVAL_MODULE` to be enabled.
 *
 * @param[in] params Environment evaluation parameters.
 *
 * @retval 0 if environment evaluation was successfully initiated.
 * @retval -EFAULT if AT command failed or feature is not supported by the modem firmware.
 * @retval -EINVAL if parameters are invalid.
 * @retval -EOPNOTSUPP if environment evaluation is not available in the current functional mode.
 */
int lte_lc_env_eval(struct lte_lc_env_eval_params *params);

/**
 * @brief Cancel an ongoing environment evaluation.
 *
 * If environment evaluation was in progress, an @ref LTE_LC_EVT_ENV_EVAL_RESULT event is received.
 *
 * @note Requires `CONFIG_LTE_LC_ENV_EVAL_MODULE` to be enabled.
 *
 * @retval 0 if environment evaluation was cancelled.
 * @retval -EFAULT if AT command failed.
 */
int lte_lc_env_eval_cancel(void);
/**
 * @brief Configure periodic searches.
 *
 * This configuration affects the periodic searches that the modem performs in limited service state
 * to obtain normal service. See @ref lte_lc_periodic_search_cfg and
 * "nRF91 AT Commands - Command Reference Guide" for more information and in-depth explanations of
 * periodic search configuration.
 *
 * @note Requires `CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` to be enabled.
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
 * @brief Get the configured periodic search parameters.
 *
 * @note Requires `CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` to be enabled.
 *
 * @param[out] cfg Periodic search configuration.
 *
 * @retval 0 if a configuration was found and populated to the provided pointer.
 * @retval -EINVAL if input parameter was @c NULL.
 * @retval -ENOENT if periodic search configuration was not set.
 * @retval -EFAULT if an AT command failed.
 * @retval -EBADMSG if the modem responded with an error to an AT command or the
 *         response could not be parsed.
 */
int lte_lc_periodic_search_get(struct lte_lc_periodic_search_cfg *const cfg);

/**
 * @brief Clear the configured periodic search parameters.
 *
 * @note Requires `CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` to be enabled.
 *
 * @retval 0 if the configuration was cleared.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 * @retval -EBADMSG if the modem responded with an error to an AT command.
 */
int lte_lc_periodic_search_clear(void);

/**
 * @brief Request an extra search.
 *
 * This can be used for example when modem is in sleep state between periodic searches. The search
 * is performed only when the modem is in sleep state between periodic searches.
 *
 * @note Requires `CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` to be enabled.
 *
 * @retval 0 if the search request was successfully delivered to the modem.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 */
int lte_lc_periodic_search_request(void);

/**
 * @brief Create a PDP context.
 *
 * In LTE terminology, this creates a PDP context (configuration) that can be activated
 * into a PDN connection. The context contains settings like APN, address family,
 * and authentication parameters.
 *
 * PDN events will be sent to all registered lte_lc event handlers.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[out] cid The context ID.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_ctx_create(uint8_t *cid);

/**
 * @brief Configure a PDP context.
 *
 * The PDN connection must be inactive when the PDP context is configured.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[in] cid The context ID.
 * @param[in] apn The Access Point Name.
 * @param[in] family The IP address family for the PDN connection.
 * @param[in] opts Additional configuration options. Optional, can be NULL.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_ctx_configure(uint8_t cid, const char *apn, enum lte_lc_pdn_family family,
			     struct lte_lc_pdn_pdp_context_opts *opts);

/**
 * @brief Set PDP context authentication parameters.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[in] cid The context ID.
 * @param[in] method The desired authentication method.
 * @param[in] user The username to use for authentication.
 * @param[in] password The password to use for authentication.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_ctx_auth_set(uint8_t cid, enum lte_lc_pdn_auth method,
				const char *user, const char *password);

/**
 * @brief Destroy a PDP context.
 *
 * The PDN connection must be inactive when the PDP context is destroyed.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[in] cid The context ID.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_ctx_destroy(uint8_t cid);

/**
 * @brief Activate a PDN connection.
 *
 * Activates the PDP context with the given CID, establishing a PDN connection.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[in] cid The context ID.
 * @param[out] esm If provided, the function will block to return the ESM error reason.
 * @param[out] family If provided, the function will block to return
 *		      @c LTE_LC_PDN_FAM_IPV4 if only IPv4 is supported, or
 *		      @c LTE_LC_PDN_FAM_IPV6 if only IPv6 is supported.
 *		      Otherwise, this value will remain unchanged.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_activate(uint8_t cid, int *esm, enum lte_lc_pdn_family *family);

/**
 * @brief Deactivate a PDN connection.
 *
 * Deactivates the PDN connection associated with the given context ID.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[in] cid The context ID.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_deactivate(uint8_t cid);

/**
 * @brief Retrieve the PDN ID for a given PDP context.
 *
 * The PDN ID can be used to route traffic through a PDN connection.
 * Multiple contexts (CIDs) may share the same PDN ID if they route through
 * the same network connection.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[in] cid The context ID.
 *
 * @return A non-negative PDN ID on success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_id_get(uint8_t cid);

/**
 * @brief Retrieve dynamic parameters of a given PDN connection.
 *
 * @param[in] cid The context ID.
 * @param[out] info PDN dynamic info.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_dynamic_info_get(uint8_t cid, struct lte_lc_pdn_dynamic_info *info);

/**
 * @brief Enable events for default PDP context, CID 0.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @retval 0 on success, otherwise negative error code.
 */
int lte_lc_pdn_default_ctx_events_enable(void);

/**
 * @brief Disable events for default PDP context, CID 0.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @retval 0 on success, otherwise negative error code.
 */
int lte_lc_pdn_default_ctx_events_disable(void);

/**
 * @brief Retrieve the default Access Point Name (APN).
 *
 * The default APN is the APN of the default PDP context (zero).
 *
 * @note Requires `CONFIG_LTE_LC_PDN_MODULE` to be enabled.
 *
 * @param[out] buf The buffer to copy the APN into. The string is null-terminated.
 * @param[in] len The size of the output buffer.
 *
 * @retval 0 On success.
 * @return A negative errno otherwise.
 */
int lte_lc_pdn_default_ctx_apn_get(char *buf, size_t len);

#if defined(CONFIG_LTE_LC_PDN_ESM_STRERROR)

/**
 * @brief Retrieve a statically allocated textual description for a given ESM error reason.
 *
 * @note Requires `CONFIG_LTE_LC_PDN_ESM_STRERROR` to be enabled.
 *
 * @param[in] reason ESM error reason.
 *
 * @return ESM error reason description.
 *         If no textual description for the given error is found,
 *         a placeholder string is returned instead.
 */
const char *lte_lc_pdn_esm_strerror(int reason);

#endif /* CONFIG_LTE_LC_PDN_ESM_STRERROR */

/**
 * @brief Configure a cellular profile.
 *
 * The cellular profile to be used for a specific access technology is decided by the modem.
 * The active cellular profile is reported in the @ref LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE
 * event if an event handler is registered.
 *
 * @note Requires `CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE` to be enabled.
 *
 * @note A cellular profile can only be configured when the modem is in functional mode
 *	 @ref LTE_LC_FUNC_MODE_POWER_OFF or @ref LTE_LC_FUNC_MODE_OFFLINE.
 *
 * @param[in] profile The cellular profile to set.
 *
 * @retval 0 on success, otherwise negative error code.
 */
int lte_lc_cellular_profile_configure(struct lte_lc_cellular_profile *profile);

/**
 * @brief Remove a cellular profile.
 *
 * This clears the configuration for the given cellular profile ID.
 *
 * @note Requires `CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE` to be enabled.
 *
 * @note A cellular profile can only be removed when the modem is in functional mode
 *	 @ref LTE_LC_FUNC_MODE_POWER_OFF or @ref LTE_LC_FUNC_MODE_OFFLINE.
 *
 * @param[in] id Cellular profile ID to remove.
 *
 * @retval 0 on success, otherwise negative error code.
 */
int lte_lc_cellular_profile_remove(uint8_t id);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* LTE_LC_H__ */
