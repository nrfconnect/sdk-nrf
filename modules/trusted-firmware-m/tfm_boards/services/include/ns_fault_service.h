/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __NS_FAULT_SERVICE_H__
#define __NS_FAULT_SERVICE_H__

#include "tfm_ioctl_api.h"

int ns_fault_service_set_handler(struct tfm_ns_fault_service_handler_context  *context,
					tfm_ns_fault_service_handler_callback callback);

void ns_fault_service_call_handler(void);

#endif /* __NS_FAULT_SERVICE_H__ */
