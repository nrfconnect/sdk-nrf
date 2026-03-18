/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_interrupts
 * @{
 * @brief CRACEN interrupt management.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#pragma once

#include <stdint.h>

void cracen_interrupts_init(void);

/**
 * @brief Cracen ISR handler.
 *
 * @param i  Unused.
 */
void cracen_isr_handler(void *i);

/**
 * @brief Blocks until interrupt for CryptoMaster is raised.
 */
uint32_t cracen_wait_for_cm_interrupt(void);

/**
 * @brief Blocks until interrupt for PK engine is raised.
 */
uint32_t cracen_wait_for_pke_interrupt(void);

/** @} */
