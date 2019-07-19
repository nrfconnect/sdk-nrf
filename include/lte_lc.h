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

/** @brief Function for requesting modem to use eDRX or disable
 * use of values defined in kconfig.
 * For reference see 3GPP 27.007 Ch. 7.40.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_lc_edrx_req(bool enable);

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
