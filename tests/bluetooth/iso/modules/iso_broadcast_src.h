/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ISO_BROADCAST_SRC_H_
#define _ISO_BROADCAST_SRC_H_

#include <stdint.h>

/** @brief Initialize the ISO broadcast source.
 *
 * @note This code is intended for CI testing and is based on a Zephyr sample.
 * Please see Zephyr ISO samples for a more implementation friendly starting point.
 *
 * @retval 0 The initialization was successful, error otherwise.
 */
int iso_broadcast_src_init(void);

#endif
