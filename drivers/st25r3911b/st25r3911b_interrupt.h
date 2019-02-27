/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ST25R3911B_INTERRUPT_H_
#define ST25R3911B_INTERRUPT_H_

#include <zephyr/types.h>
#include <kernel.h>

/**
 * @file
 * @defgroup st25r3911b_interrupts ST25R3911B NFC Reader Interrupts
 * @{
 *
 * @brief API for the ST25R3911B NFC Reader Interrupts.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup st25r3911b_irq_def ST25R3911B Interrupts Definition
 * @{
 * @brief Interrupts are grouped in three registers.
 */
#define ST25R3911B_IRQ_MASK_ALL             (0xFFFFFFUL)
#define ST25R3911B_IRQ_MASK_NONE            (0UL)
#define ST25R3911B_IRQ_MASK_OSC             (0x80UL)
#define ST25R3911B_IRQ_MASK_FWL             (0x40UL)
#define ST25R3911B_IRQ_MASK_RXS             (0x20UL)
#define ST25R3911B_IRQ_MASK_RXE             (0x10UL)
#define ST25R3911B_IRQ_MASK_TXE             (0x08UL)
#define ST25R3911B_IRQ_MASK_COL             (0x04UL)
#define ST25R3911B_IRQ_MASK_TIM             (0x02UL)
#define ST25R3911B_IRQ_MASK_ERR             (0x01UL)

#define ST25R3911B_IRQ_MASK_DCT             (0x8000UL)
#define ST25R3911B_IRQ_MASK_NRE             (0x4000UL)
#define ST25R3911B_IRQ_MASK_GPE             (0x2000UL)
#define ST25R3911B_IRQ_MASK_EON             (0x1000UL)
#define ST25R3911B_IRQ_MASK_EOF             (0x0800UL)
#define ST25R3911B_IRQ_MASK_CAC             (0x0400UL)
#define ST25R3911B_IRQ_MASK_CAT             (0x0200UL)
#define ST25R3911B_IRQ_MASK_NFCT            (0x0100UL)

#define ST25R3911B_IRQ_MASK_CRC             (0x800000UL)
#define ST25R3911B_IRQ_MASK_PAR             (0x400000UL)
#define ST25R3911B_IRQ_MASK_ERR2            (0x200000UL)
#define ST25R3911B_IRQ_MASK_ERR1            (0x100000UL)
#define ST25R3911B_IRQ_MASK_WT              (0x080000UL)
#define ST25R3911B_IRQ_MASK_WAM             (0x040000UL)
#define ST25R3911B_IRQ_MASK_WPH             (0x020000UL)
#define ST25R3911B_IRQ_MASK_WCAP            (0x010000UL)

/**
 * @}
 */

/** @brief Initialize NFC Reader interrupts
 *
 *  @details NFC reader informs about interruptions by
 *           a rising edge on the pin. This function initialize
 *           GPIO interface to handle it.
 *
 *  @param[in] irq_sem Semaphore needed to synchronized
 *                     processing interrupts from other context.
 *
 *  @return Returns 0 if initialization was successful,
 *          otherwise negative value.
 */
int st25r3911b_irq_init(struct k_sem *irq_sem);

/** @brief Modify NFC Reader interrupts.
 *
 *  @details This function can modify currently enabled interrupts.
 *           The setting mask covers the clearing mask so firstly
 *           interrupts from clear mask are disabled and then interrupts
 *           from set mask are enabled. The mask can be composed of several
 *           interrupts. For example mask = ST25R3911B_IRQ_MASK_PAR |
 *           ST25R3911B_IRQ_MASK_CRC.
 *           Available interrupts @ref st25r3911b_irq_def.
 *
 *  @param[in] clr_mask Interrupts clear mask;
 *  @param[in] set_mask Interrupts set mask.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_irq_modify(u32_t clr_mask, u32_t set_mask);

/** @brief Enable NFC Reader interrupts.
 *
 *  @details Interrupts mask can be composed of several interrupts
 *           @ref st25r3911b_irq_def
 *
 *  @param[in] mask Interrupts to enable mask.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_irq_enable(u32_t mask);

/** @brief Disable NFC Reader interrupts.
 *
 *  @details Interrupts mask can be composed of several interrupts
 *           @ref st25r3911b_irq_def
 *
 *  @param[in] mask Interrupts to disable mask.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_irq_disable(u32_t mask);

/** @brief Clear NFC Reader interrupts status.
 *
 *  @details Interrupts are cleared by reading NFC Reader
 *           interrupts registers.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r39_irq_clear(void);

/** @brief Wait for NFC Reader interrupts.
 *
 *  @details Function can wait for one interrupt or
 *           several defined by mask.
 *
 *  @param[in] mask Interrupts mask @ref st25r3911b_irq_def.
 *  @param[in] timeout Wait time in milliseconds.
 *
 *  @return Interrupts status, reading after timeout or when
 *          interrupt occurred.
 */
u32_t st25r3911b_irq_wait_for_irq(u32_t mask, s32_t timeout);

/** @brief Read NFC Reader interrupts status.
 *
 *  @details Three interrupts status registers are read and
 *           then their status is returned.
 *
 *  @return Interrupts status.
 */
u32_t st25r3911b_irq_read(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_INTERRUPT_H_ */
