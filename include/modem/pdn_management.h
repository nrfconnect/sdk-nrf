/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file pdn_management.h
 *
 * @brief Public APIs for the PDN management library.
 */

#ifndef PDN_MANAGEMENT_H_
#define PDN_MANAGEMENT_H_


/**@brief  Initialize and connect PDN to APN 'apn_name'
 *
 * @param[in] apn_name Identifies the APN name for which the PDN connection
 *                     is requested.
 *
 * @retval -1 in case of failure, else, an fd identifying the PDN connection.
 */
int pdn_init_and_connect(char *apn_name);

/**@brief Diconnect the  PDN connection.
 *
 * @param[in] pdn_fd Identifies the PDN for which the procedure is requested.
 */
void pdn_disconnect(int pdn_fd);

#endif /* PDN_MANAGEMENT_H_ */
