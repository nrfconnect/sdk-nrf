/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: communication interfaces */

#ifndef COMM_PROC_H__
#define COMM_PROC_H__

#include <stdint.h>
#include <zephyr/kernel.h>

/**< Maximum expected size of received packet */
#define COMM_MAX_TEXT_DATA_SIZE (320u)

void comm_init(void);

#endif /* COMM_PROC_H__ */
