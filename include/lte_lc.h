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
 *	 as returned by the AT command "AT+CFUN?".
 */
enum lte_lc_func_mode {
	LTE_LC_FUNC_MODE_POWER_OFF		= 0,
	LTE_LC_FUNC_MODE_NORMAL			= 1,
	LTE_LC_FUNC_MODE_OFFLINE		= 4,
	LTE_LC_FUNC_MODE_OFFLINE_UICC_ON	= 44
};

/** @brief Function for initializing
 * the modem.  NOTE: a follow-up call to lte_lc_connect()
 * must be made.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_init(void);

/** @brief Function to make a connection with the modem.
 * NOTE: prior to calling this function a call to lte_lc_init()
 * must be made.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_connect(void);

/** @brief Function for initializing
 * and make a connection with the modem
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_init_and_connect(void);

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
