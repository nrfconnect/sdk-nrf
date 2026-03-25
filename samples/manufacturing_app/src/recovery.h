/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file recovery.h
 *
 * Recovery option availability and execution-suspend helper.
 *
 * When the manufacturing application encounters an unrecoverable error it
 * logs the available recovery methods and then suspends execution. This
 * module encapsulates the logic for determining and logging those options.
 */

#ifndef RECOVERY_H_
#define RECOVERY_H_

#include <stdbool.h>

/**
 * @brief Log the currently available recovery methods and suspend execution.
 *
 * Prints:
 *   Try to recover the device.
 *   Erase-all: available/unavailable (reason)
 *   Authenticated erase-all: available/unavailable (reason)
 *   Execution suspended.
 *
 * Then enters an infinite loop. The device must be power-cycled or reset
 * externally after the operator has taken corrective action.
 *
 * @param keys_provisioned  Pass true if secure-boot/ADAC keys have already
 *                          been provisioned (makes authenticated erase-all
 *                          potentially available).
 */
__attribute__((noreturn))
void recovery_suspend(bool keys_provisioned);

#endif /* RECOVERY_H_ */
