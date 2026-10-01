/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Maps the RTOS-agnostic settings used by the generated
 * mpsl_log_msg.h / sdc_log_msg.h tables onto Zephyr Kconfig.
 */

#ifndef MPSL_LOG_CONFIG_H__
#define MPSL_LOG_CONFIG_H__

#define MPSL_LOG_PRINT_LEVEL          CONFIG_MPSL_LOG_PRINT_LEVEL
#define SDC_LOG_PRINT_LEVEL           CONFIG_MPSL_LOG_PRINT_LEVEL

#endif /* MPSL_LOG_CONFIG_H__ */
