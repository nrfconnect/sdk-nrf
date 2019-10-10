/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file pdn.h
 *
 * @brief Public APIs for the PDN management library.
 */

#ifndef PDN_H_
#define PDN_H_

/**@brief Connect to a packet data network.
 *
 * Creates a PDN socket and connect to an access point, if necessary.
 *
 * @param[in,out]	fd	The socket handle.
 * @param[in]		apn	The packet data network name.
 *
 * @return 0 if PDN socket was valid and already connected.
 * @return 1 if PDN socket has been recreated.
 * @return -1 on error.
 */
int pdn_activate(int *fd, const char *apn);

#endif /* PDN_H_ */

