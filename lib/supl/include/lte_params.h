/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUPL_LTE_PARAMS_H_
#define SUPL_LTE_PARAMS_H_

#include "supl_session.h"

/**
 * @brief Initialize LTE parameters struct
 *
 * @param[in,out]   params  LTE parameters to initialize
 */
void lte_params_init(struct lte_params *params);

/**
 * @brief Handler for XMONITOR AT command
 *
 * @param[in,out] session_ctx   session context to fill
 * @param[in]     col_number    column number
 * @param[in]     buf           input buffer
 * @param[in]     buf_len       input buffer length
 *
 * @return 0 for success, -1 otherwise
 */
int handler_at_xmonitor(struct supl_session_ctx *session_ctx,
			int col_number,
			const char *buf,
			int buf_len);

/**
 * @brief Initialize the device ID
 *
 * @param[in,out]   session_ctx  The session context
 */
void device_id_init(struct supl_session_ctx *session_ctx);

/**
 * @brief Handler for +CGDCONT AT response
 *
 * @param[in,out] session_ctx   session context to fill
 * @param[in]     col_number    column number
 * @param[in]     buf           input buffer
 * @param[in]     buf_len       input buffer length
 *
 * @return 0 for success, -1 otherwise
 */
int handler_at_cgdcont(struct supl_session_ctx *session_ctx,
		       int col_number,
		       const char *buf,
		       int buf_len);

#endif /* SUPL_LTE_PARAMS_H_ */
