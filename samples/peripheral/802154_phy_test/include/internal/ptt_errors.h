/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: PHY Test Tool library errors declaration */

#ifndef PTT_ERRORS_H__
#define PTT_ERRORS_H__

#include <stdint.h>

/** error codes */
enum ptt_ret {
	PTT_RET_SUCCESS = 0, /**< success */
	PTT_RET_NULL_PTR, /**< pointer is NULL */
	PTT_RET_INVALID_VALUE, /**< invalid input value */
	PTT_RET_INVALID_STATE, /**< invalid state */
	PTT_RET_INVALID_MODE, /**< invalid mode of device */
	PTT_RET_INVALID_COMMAND, /**< invalid command*/
	PTT_RET_NO_FREE_SLOT, /**< no free slots in a pool */
	PTT_RET_BUSY, /**< resource is busy */
};

#endif /* PTT_ERRORS_H__ */
