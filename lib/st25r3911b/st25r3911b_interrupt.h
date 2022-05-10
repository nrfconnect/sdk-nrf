/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ST25R3911B_INTERRUPT_H_
#define ST25R3911B_INTERRUPT_H_

#include <zephyr/types.h>
#include <zephyr/kernel.h>

/**
 * @file
 * @defgroup st25r3911b_interrupts ST25R3911B NFC Reader interrupts
 * @{
 *
 * @brief API for the ST25R3911B NFC Reader interrupts.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup st25r3911b_irq_def ST25R3911B interrupts definition
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

/** @brief Initialize NFC Reader interrupts.
 *
 *  @details The NFC Reader informs about interruptions by
 *           a rising edge on the pin. This function initializes
 *           the GPIO interface to handle this.
 *
 *  @param[in] irq_sem Semaphore needed to synchronize
 *                     processing interrupts from other context.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_irq_init(struct k_sem *irq_sem);

/** @brief Modify NFC Reader interrupts.
 *
 *  @details This function modifies the currently enabled interrupts.
 *           The set mask covers the clear mask, which means that first,
 *           interrupts from the clear mask are disabled, and then interrupts
 *           from the set mask are enabled. The mask can be composed of several
 *           interrupts. For example: <tt>mask = ST25R3911B_IRQ_MASK_PAR |
 *           ST25R3911B_IRQ_MASK_CRC</tt>
 *
 *           See @ref st25r3911b_irq_def for available interrupts.
 *
 *  @param[in] clr_mask Interrupts clear mask.
 *  @param[in] set_mask Interrupts set mask.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_irq_modify(uint32_t clr_mask, uint32_t set_mask);

/** @brief Enable NFC Reader interrupts.
 *
 *  @details The interrupts mask can be composed of several interrupts
 *           (see @ref st25r3911b_irq_def).
 *
 *  @param[in] mask Interrupts to enable mask.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_irq_enable(uint32_t mask);

/** @brief Disable NFC Reader interrupts.
 *
 *  @details The interrupts mask can be composed of several interrupts
 *           (see @ref st25r3911b_irq_def).
 *
 *  @param[in] mask Interrupts to disable mask.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_irq_disable(uint32_t mask);

/** @brief Clear NFC Reader interrupts status.
 *
 *  @details Interrupts are cleared by reading NFC Reader
 *           interrupts registers.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r39_irq_clear(void);

/** @brief Wait for NFC Reader interrupts.
 *
 *  @details This function can wait for one interrupt or
 *           for several (defined by a mask). It returns
 *           after an interrupt occurs or after a time-out.
 *
 *  @param[in] mask Interrupts mask (see @ref st25r3911b_irq_def).
 *  @param[in] timeout Wait time in milliseconds.
 *
 *  @return Interrupts status.
 */
uint32_t st25r3911b_irq_wait_for_irq(uint32_t mask, int32_t timeout);

/** @brief Read NFC Reader interrupts status.
 *
 *  @details Three interrupts status registers are read and
 *           their status is returned.
 *
 *  @return Interrupts status.
 */
uint32_t st25r3911b_irq_read(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_INTERRUPT_H_ */
