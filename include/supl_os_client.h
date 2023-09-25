/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUPL_OS_CLIENT_H_
#define SUPL_OS_CLIENT_H_

#include <nrf_modem_gnss.h>
#include <supl_session.h>

/**
 * @file supl_os_client.h
 *
 * @defgroup supl_os SUPL OS client
 *
 * @{
 **/

/**
 *
 * @brief Start a SUPL session
 *
 * @param[in] agnss_request This contain the information about the A-GNSS data
 *                          that the GNSS module is requesting from the server.
 *
 * @return 0  SUPL session was successful.
 *         <0 SUPL session failed.
 */
int supl_session(const struct nrf_modem_gnss_agnss_data_frame *const agnss_request);

/**
 * @brief Setup the API and the buffers required by
 *        the SUPL client library.
 *
 * @param[in] api Function pointers to the API required by SUPL client library
 *                to work.
 *
 * @return 0  SUPL client library was initialized successfully.
 *         <0 Failed to initialize the SUPL client library.
 */
int supl_init(const struct supl_api *const api);

/** @} */

#endif /* SUPL_OS_CLIENT_H_ */
