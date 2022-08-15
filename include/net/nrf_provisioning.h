/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file nrf_provisioning.h
 *
 * @brief nRF Provisioning API.
 */
#ifndef NRF_PROVISIONING_H__
#define NRF_PROVISIONING_H__

#include <stddef.h>

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup nrf_provisioning nRF Device Provisioning API
 *  @{
 */

/**
 * @typedef nrf_provisioning_mmode_cb_t
 * @brief Callback to request a modem state change, being it powering off, flight mode etc.
 *
 * @param new_mode New mode.
 * @param user_data Not used.
 * @return <0 on error,  previous mode on success.
 */
typedef int (*nrf_provisioning_mmode_cb_t)(enum lte_lc_func_mode new_mode, void *user_data);

/**
 * @struct nrf_provisioning_mm_change
 * @brief Holds the callback used for querying permission from the app to proceed when modem's state
 * changes. Together with data set by the callback provider.
 *
 * @param cb        The callback function.
 * @param user_data App specific data to be fed to the callback once it's called.
 */
struct nrf_provisioning_mm_change {
	nrf_provisioning_mmode_cb_t cb;
	void *user_data;
};

/**
 * @typedef nrf_provisioning_dmode_cb_t
 * @brief Once provisioning is done this callback is to be called.
 *
 * @return <0 on error,  previous mode on success.
 */
typedef void (*nrf_provisioning_dmode_cb_t)(void *user_data);

/**
 * @struct nrf_provisioning_dm_change
 * @brief Holds the callback to be called once provisioning is done together with data set by the
 * callback provider.
 *
 * @param cb        The callback function.
 * @param user_data App specific data to be fed to the callback once it's called.
 */
struct nrf_provisioning_dm_change {
	nrf_provisioning_dmode_cb_t cb;
	void *user_data;
};

/**
 * @brief Initializes the provisioning library and registers callback handlers.
 *
 * Consequent calls will only change callback functions used.
 * Feeding a null as a callback address means that the corresponding default callback function is
 * taken into use.
 *
 * @param mmode Modem mode change callback. Used when data is written to modem.
 * @param dmode Provisioning done callback. Used when all provisioning commands have been executed
 *              and responded to successfully.
 * @return <0 on error, 0 on success.
 */
int nrf_provisioning_init(struct nrf_provisioning_mm_change *mmode,
				struct nrf_provisioning_dm_change *dmode);

/**
 * @brief Starts provisioning immediately.
 *
 * @retval -EFAULT if the library is uninitialized.
 * @retval 0 on success.
 */
int nrf_provisioning_trigger_manually(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_H__ */
