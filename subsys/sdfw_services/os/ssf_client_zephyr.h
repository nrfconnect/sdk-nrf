/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_CLIENT_ZEPHYR_H__
#define SSF_CLIENT_ZEPHYR_H__

#include "ssf_client_os.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SSF_CLIENT_LOG_REGISTER(module, log_lvl_cfg) LOG_MODULE_REGISTER(module, log_lvl_cfg)
#define SSF_CLIENT_LOG_DECLARE(module, log_lvl_cfg) LOG_MODULE_DECLARE(module, log_lvl_cfg)

#define SSF_CLIENT_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define SSF_CLIENT_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define SSF_CLIENT_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define SSF_CLIENT_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

struct ssf_client_sem {
	struct k_sem sem;
};

#endif /* SSF_CLIENT_ZEPHYR_H__ */
