/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *  @defgroup zb_error ZBOSS error handling utilities
 *  @{
 */

/**
 * @file
 * @brief This file defines the error handler for ZBOSS error codes.
 */

#ifndef ZIGBEE_ERROR_HANDLER_H__
#define ZIGBEE_ERROR_HANDLER_H__

#include <kernel.h>

#include <zboss_api.h>
#include <zb_errors.h>

#define ZB_ERROR_BASE_NUM 20000 /*< ZBOSS stack error codes base. */

#ifdef CONFIG_ZBOSS_ERROR_PRINT_TO_LOG
#include "zb_error_to_string.h"
#include <logging/log.h>


/**@brief Macro for calling error handler function if supplied
 *        error code any other than RET_OK.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define ZB_ERROR_CHECK(ERR_CODE)					\
	do {								\
		const uint32_t LOCAL_ERR_CODE = (uint32_t) (-ERR_CODE);	\
		if (LOCAL_ERR_CODE != RET_OK) {				\
			LOG_ERR("ERROR %u [%s] at %s:%u",		\
				LOCAL_ERR_CODE,				\
				zb_error_to_string_get(LOCAL_ERR_CODE),	\
				__FILE__,				\
				__LINE__);				\
			k_fatal_halt(K_ERR_KERNEL_PANIC);		\
		}							\
	} while (0)

/**@brief Macro for calling error handler function
 *        if bdb_start_top_level_commissioning return code
 *        indicates that it BDB procedure did not succeed.
 *
 * @param[in] COMM_STATUS Value returned by
 *            bdb_start_top_level_commissioning function.
 */
#define ZB_COMM_STATUS_CHECK(COMM_STATUS)				      \
	do {								      \
		if (COMM_STATUS != ZB_TRUE) {				      \
			LOG_ERR("Unable to start BDB commissioning at %s:%u", \
				__FILE__,				      \
				__LINE__);				      \
			ZB_ERROR_CHECK(RET_ERROR);			      \
		}							      \
	} while (0)
#else
#define ZB_ERROR_CHECK(ERR_CODE)				  \
	do {							  \
		const uint32_t LOCAL_ERR_CODE = (uint32_t) (-ERR_CODE); \
		if (LOCAL_ERR_CODE != RET_OK) {			  \
			k_fatal_halt(K_ERR_KERNEL_PANIC);	  \
		}						  \
	} while (0)

#define ZB_COMM_STATUS_CHECK(COMM_STATUS)	   \
	do {					   \
		if (COMM_STATUS != ZB_TRUE) {	   \
			ZB_ERROR_CHECK(RET_ERROR); \
		}				   \
	} while (0)
#endif

/**
 * @}
 *
 */

#endif /* ZIGBEE_ERROR_HANDLER_H__ */
