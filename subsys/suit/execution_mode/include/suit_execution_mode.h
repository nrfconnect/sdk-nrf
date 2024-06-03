/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_EXECUTION_MODE_H__
#define SUIT_EXECUTION_MODE_H__

#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	EXECUTION_MODE_STARTUP = 0,
	EXECUTION_MODE_INVOKE,
	EXECUTION_MODE_INVOKE_RECOVERY,
	EXECUTION_MODE_INSTALL,
	EXECUTION_MODE_INSTALL_RECOVERY,
	EXECUTION_MODE_POST_INVOKE,
	EXECUTION_MODE_POST_INVOKE_RECOVERY,
	EXECUTION_MODE_FAIL_NO_MPI,
	EXECUTION_MODE_FAIL_MPI_INVALID,
	EXECUTION_MODE_FAIL_MPI_INVALID_MISSING,
	EXECUTION_MODE_FAIL_MPI_UNSUPPORTED,
	EXECUTION_MODE_FAIL_INVOKE_RECOVERY,
	EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP,
} suit_execution_mode_t;

/**
 * @brief Return current execution mode set in module
 *
 * @return suit_execution_mode_t Current value
 */
suit_execution_mode_t suit_execution_mode_get(void);

/**
 * @brief Set new execution mode
 *
 * @param mode Valid execution mode
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid mode
 */
suit_plat_err_t suit_execution_mode_set(suit_execution_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_EXECUTION_H__ */
