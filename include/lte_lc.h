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

#endif /* ZEPHYR_INCLUDE_LTE_LINK_CONTROL_H_ */
