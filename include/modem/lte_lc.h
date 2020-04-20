/**
 * @file lte_lc.h
 *
 * @brief Public APIs for the LTE Link Control driver.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
	LTE_LC_SYSTEM_MODE_NBIOT_GPS
};

/* NOTE: enum lte_lc_func_mode maps directly to the functional mode
 *	 as returned by the AT command "AT+CFUN?" as specified in
 *	 "nRF91 AT Commands - Command Reference Guide v1.1"
 */
enum lte_lc_func_mode {
	LTE_LC_FUNC_MODE_POWER_OFF		= 0,
	LTE_LC_FUNC_MODE_NORMAL			= 1,
	LTE_LC_FUNC_MODE_OFFLINE		= 4,
	LTE_LC_FUNC_MODE_DEACTIVATE_LTE		= 20,
	LTE_LC_FUNC_MODE_ACTIVATE_LTE		= 21,
	LTE_LC_FUNC_MODE_DEACTIVATE_GNSS	= 30,
	LTE_LC_FUNC_MODE_ACTIVATE_GNSS		= 31,
	LTE_LC_FUNC_MODE_OFFLINE_UICC_ON	= 44,
};

enum lte_lc_evt_type {
	LTE_LC_EVT_NW_REG_STATUS,
	LTE_LC_EVT_PSM_UPDATE,
	LTE_LC_EVT_EDRX_UPDATE,
	LTE_LC_EVT_RRC_UPDATE,
	LTE_LC_EVT_CELL_UPDATE,
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
	u32_t id;	/* E-UTRAN cell ID */
	u32_t tac;	/* Tracking Area Code */
};

struct lte_lc_evt {
	enum lte_lc_evt_type type;
	union {
		enum lte_lc_nw_reg_status nw_reg_status;
		enum lte_lc_rrc_mode rrc_mode;
		struct lte_lc_psm_cfg psm_cfg;
		struct lte_lc_edrx_cfg edrx_cfg;
		struct lte_lc_cell cell;
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
 * 	 made to establish an LTE connection.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_init(void);

/** @brief Function to make a connection with the modem.
 *
 * @note prior to calling this function a call to @ref lte_lc_init
 *	 must be made, otherwise -EPERM is returned.
 *
 * @return Zero on success, -EPERM if the module has not been initialized,
 *	   otherwise a (negative) error code.
 */
int lte_lc_connect(void);

/**@brief Initializes the LTE module, configures the modem and connects to LTE
 *	  network. The function blocks until connection is established, or
 *	  the connection attempt times out.
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
 * @param handler Event handler for receiving LTE events. The parameter can be
 *		  NULL if an event handler is already registered.
 *
 * @return Zero on success, -EINVAL if no handler is provided and not already
 *	   registered, otherwise a (negative) error code.
 */
int lte_lc_init_and_connect_async(lte_lc_evt_handler_t handler);

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

/** @brief Function for requesting modem to go to or disable
 * power saving mode (PSM) with default settings defined in kconfig.
 * For reference see 3GPP 27.007 Ch. 7.38.
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

/** @brief Function for requesting modem to use eDRX or disable
 * use of values defined in kconfig.
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_edrx_req(bool enable);

/**@brief Get the current network registration status.
 *
 * @param status Pointer for network registation status.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status);

/**@brief Set the modem's system mode.
 *
 * @param mode System mode to set.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_system_mode_set(enum lte_lc_system_mode mode);

/**@brief Get the modem's system mode.
 *
 * @param mode Pointer to system mode variable.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_system_mode_get(enum lte_lc_system_mode *mode);

/**@brief Get the modem's functional mode.
 *
 * @param mode Pointer to functional mode variable.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_func_mode_get(enum lte_lc_func_mode *mode);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
