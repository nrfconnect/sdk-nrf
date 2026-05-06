/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LOG_MEMORY_PROTECTION_H
#define __LOG_MEMORY_PROTECTION_H

void log_memory_protection_of_mpu(void);
void log_memory_protection_of_spu_nsc(void);
void log_memory_protection_of_spu_flash(void);
void log_memory_protection_of_spu_ram(void);
void log_memory_protection_of_spu(void);
void log_memory_protection_mpu_spu(void);

void log_memory_protection_sau_mpc(void);

#endif /* __LOG_MEMORY_PROTECTION_H */
