/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TFM_NS_FAULT_H
#define TFM_NS_FAULT_H

#include <tfm_ioctl_api.h>

void tfm_ns_fault_handler_callback(void);

struct tfm_ns_fault_service_handler_context *tfm_ns_fault_get_context(void);

#endif /* TFM_NS_FAULT_H */
