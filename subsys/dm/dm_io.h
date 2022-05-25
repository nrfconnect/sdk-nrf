/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DM_IO_H_
#define DM_IO_H_

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum dm_io_output {
	DM_IO_RANGING,
	DM_IO_ADD_REQUEST,
};

int dm_io_init(void);
void dm_io_set(enum dm_io_output out);
void dm_io_clear(enum dm_io_output out);

#ifdef __cplusplus
}
#endif


#endif /* DM_IO_H_ */
