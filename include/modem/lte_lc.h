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

/** LTE mode. Used to indicate which LTE mode is currently active if more than
 *  one mode is enabled in the system mode configuration.
 *  The values for LTE-M and NB-IoT correspond to the values for the AcT field
 *  in a AT+CEREG response.
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
};

enum lte_lc_rrc_mode {
	LTE_LC_RRC_MODE_IDLE,
	LTE_LC_RRC_MODE_CONNECTED,
};

struct lte_lc_psm_cfg {
	int tau;		/* Periodic Tracking Area Update interval */
	int active_time;	/* Active-time (time from RRC idle to PSM) */
};

struct lte_lc_edrx_cfg {
	float edrx;	/* eDRX interval value [s] */
	float ptw;	/* Paging time window [s] */
};

struct lte_lc_cell {
	uint32_t id;	/* E-UTRAN cell ID */
	uint32_t tac;	/* Tracking Area Code */
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
	};
};

typedef void(*lte_lc_evt_handler_t)(const struct lte_lc_evt *const evt);

/* NOTE: enum order is important and should be preserved. */
enum lte_lc_pdp_type {
	LTE_LC_PDP_TYPE_IP = 0,
	LTE_LC_PDP_TYPE_IPV6,
	LTE_LC_PDP_TYPE_IPV4V6
};

enum lte_lc_pdn_auth_type {
	LTE_LC_PDN_AUTH_TYPE_NONE = 0,
	LTE_LC_PDN_AUTH_TYPE_PAP = 1,
	LTE_LC_PDN_AUTH_TYPE_CHAP = 2,
};

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
 *	   eDRX is requested using `lte_lc_edrx_req`.
 *	   For reference see subclause 10.5.5.32 of 3GPP TS 24.008.
 *
 * @param ptw Paging Time Window value as null-terminated string.
 *        Set NULL to use manufacturer-specific default value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_ptw_set(const char *ptw);

/** @brief Function for setting modem eDRX value to be used when
 * eDRX is subsequently enabled using `lte_lc_edrx_req`.
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @param edrx eDRX value as null-terminated string.
 *        Set NULL to use manufacturer-specific default.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_edrx_param_set(const char *edrx);

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

/**
 * @brief Set the parameters for the default PDP context.
 * Must be called prior to `lte_lc_init` or `lte_lc_init_and_connect`.
 *
 * @param[in]  type            One of enum lte_pdp_type
 * @param[in]  apn             null terminated APN string
 * @param[in]  ip4_addr_alloc  0 - Allocate IPV4 address via NAS signaling
 *	(default), 1 - Allocate IPV4 address via DHCP.
 * @param[in]  nslpi           0 - NSLPI value from configuration is used
 *	(default), 1 - Value "Not configured".
 * @param[in]  secure_pco      0 - Protected PCO transmission is not requested
 *	(default), 1 - Protected PCO transmission is requested
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_pdp_context_set(enum lte_lc_pdp_type type, const char *apn,
			   bool ip4_addr_alloc, bool nslpi, bool secure_pco);

/**
 * @brief Set PDN authentication parameters for the default context.
 * Must be called prior to `lte_lc_init` or `lte_lc_init_and_connect`.
 *
 * @param[in]  auth_prot  One of enum lte_lc_pdn_auth_type
 * @param[in]  username   Null terminated username string
 * @param[in]  password   Null terminated password string
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_pdn_auth_set(enum lte_lc_pdn_auth_type auth_prot,
			const char *username, const char *password);

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

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
