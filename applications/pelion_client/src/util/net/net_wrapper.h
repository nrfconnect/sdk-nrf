/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NET_WRAPPER_
#define NET_WRAPPER_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the network
 *
 * The idea is that after the function is called the network transport
 * layer is configured and ready to be used by the pelion.
 *
 * @return 0 if success or error code.
 */
int net_wrapper_init(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_WRAPPER_ */
