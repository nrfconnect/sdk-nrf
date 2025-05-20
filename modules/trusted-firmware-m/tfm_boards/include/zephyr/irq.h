/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <nrfx.h>

/*
 * There is no way to translate from Zephyr's IRQ_CONNECT call to TF-M
 * so we compile out such invocations. The user must ensure that the
 * IRQ given to IRQ_CONNECT is connected correctly in TF-M.
 */
#define IRQ_CONNECT(arg0, arg1, arg2, arg3, arg4)

#define irq_disable(irq) NVIC_DisableIRQ(irq)

#define irq_enable(irq) NVIC_EnableIRQ(irq)
