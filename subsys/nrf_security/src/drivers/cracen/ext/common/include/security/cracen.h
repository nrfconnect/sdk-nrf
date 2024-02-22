/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup security Security
 * @brief Cracen security
 *
 * @{
 */

#ifndef CRACEN_H_
#define CRACEN_H_

#include <stdint.h>
#include <stdbool.h>
#include <nrf.h>

#include <hal/nrf_cracen.h>

#ifndef NRF_CRACENCORE_BASE
/** @brief A portable symbol to a typed NRF_CRACENCORE.
 *
 * The MDK provides a portable symbol to a typed NRF_CRACENCORE, but
 * not to a base address. So we create our own portable symbol.
 */
#define NRF_CRACENCORE_BASE ((uint32_t)NRF_CRACENCORE)
#endif

/** @brief Base address of ba414ep register interface */
#define CRACEN_ADDR_BA414EP_REGS_BASE ((uint32_t)(&NRF_CRACENCORE->PK))

/** @brief Base address of ba414ep crypto RAM */
#define CRACEN_ADDR_BA414EP_CRYPTORAM_BASE ((uint32_t)(NRF_CRACENCORE_BASE + 0x8000))

/** @brief Address of the SilexPK MicroCode */
#define CRACEN_SX_PK_MICROCODE_ADDRESS ((uint32_t)(NRF_CRACENCORE_BASE + 0xC000))

/** @brief TODO: Replace with define from the MDK of the same name when available */
#define KMU_KEYSLOT_BITS 128

/** @brief Number of KMU slots required for the IKG seed.
 *
 * Calculated to be the size of the seed divided by the size of each key slot.
 *
 * 3 = 384 bits / 128 bit
 */
#define IKG_SEED_KMU_SLOT_NUM ((CRACEN_SEED_MaxCount * 32) / KMU_KEYSLOT_BITS)

/**
 * @brief Initialize CRACEN.
 *
 */
int cracen_init(void);

/**
 * @brief Increase the count of users of the CRACEN hw peripheral. Operation must call
 *        @ref cracen_release when operation is finished. The CRACEN hw peripheral will
 *        be powered when number of users > 0.
 */
void cracen_acquire(void);

/**
 * @brief Release the CRACEN hw peripheral.
 *
 * If there are no more users Cracen will be powered off, it's events
 * will be cleared, it's IRQs will be disabled, the pending IRQs in
 * the NVIC will be cleared, and interrupts from CRACEN will be
 * disabled in the NVIC.
 */
void cracen_release(void);

/**
 * @}
 */

#endif
