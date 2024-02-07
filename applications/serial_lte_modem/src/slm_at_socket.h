/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_SOCKET_
#define SLM_AT_SOCKET_

/**@file slm_at_socket.h
 *
 * @brief Vendor-specific AT command for Socket service.
 * @{
 */

/**
 * @brief Set SLM socket option.
 * @param[in] socket Socket to set the option.
 * @param[in] option Socket option to set.
 * @param[in] value Value of the socket option.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_sockopt_set(int socket, int option, int value);

/**
 * @brief Initialize socket AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_socket_init(void);

/**
 * @brief Uninitialize socket AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_socket_uninit(void);
/** @} */

#endif /* SLM_AT_SOCKET_ */
