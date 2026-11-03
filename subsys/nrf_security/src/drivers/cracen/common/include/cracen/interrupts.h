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

/* Mask and unmask the CryptoMaster interrupt for polling short jobs (see
 * sx_cmdma_check()). It is masked when a job starts, so a job that ends while
 * polled raises no interrupt, and unmasked only before sleeping on the job. A
 * job that ended while masked still has its event latched and interrupts as
 * soon as it is unmasked.
 */
void cracen_cm_interrupt_mask(void);
void cracen_cm_interrupt_unmask(void);

/* Take the interrupt of a job that ended while polled, after clearing its
 * CryptoMaster status. Clears the latched event, so it doesn't interrupt when a
 * later job unmasks it, and returns any status the ISR posted (or 0), so it
 * can't be taken for the end of a later job.
 */
uint32_t cracen_cm_interrupt_take(void);

/**
 * @brief Blocks until interrupt for PK engine is raised.
 */
uint32_t cracen_wait_for_pke_interrupt(void);

/** @} */
