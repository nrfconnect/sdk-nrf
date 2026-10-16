/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Test-only fault-injection API of the TAC5112 I2C emulator.
 *
 * The emulator models the codec's latched fault registers (INT_LTCH0,
 * OUT_CH1_LTCH, OUT_CH2_LTCH, which clear on read) and, when the codec's
 * devicetree node has a @c fault-gpios property backed by a
 * @c zephyr,gpio-emul controller, the codec's IRQ output line. The line is
 * asserted while any *unmasked* latched event, or the brown-out status, is
 * pending and de-asserted once reading the latch registers has cleared them.
 */

#ifndef TAC5112_EMUL_H_
#define TAC5112_EMUL_H_

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/drivers/emul.h>

#include "tac5112_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OR @p bits into a latched fault register and update the IRQ line.
 *
 * @param target Emulator instance (EMUL_DT_GET()).
 * @param reg    One of TAC5112_REG_INT_LTCH0, TAC5112_REG_OUT_CH1_LTCH or
 *               TAC5112_REG_OUT_CH2_LTCH.
 * @param bits   Latch bits to set.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p reg is not a latched fault register.
 */
int tac5112_emul_inject_latch(const struct emul *target, struct tac5112_reg reg, uint8_t bits);

/**
 * @brief Set or clear the brown-out shutdown status (AVDD_IOVDD_STS).
 *
 * This is a live status bit: it is not cleared by reading and stays set until
 * the test clears it with @c active = false.
 */
void tac5112_emul_set_brownout(const struct emul *target, bool active);

/**
 * @brief Make I2C writes to one register fail with -EIO (or stop doing so).
 *
 * Lets tests exercise the driver's error paths, including partial failure of
 * multi-register updates. Writes to other registers are unaffected.
 */
void tac5112_emul_fail_writes(const struct emul *target, struct tac5112_reg reg, bool fail);

/**
 * @brief Let @p skip writes to a register succeed, then fail all later ones.
 *
 * Allows a sequence such as "first write succeeds, rollback write fails".
 * At most two registers can have a write-failure hook at once.
 */
void tac5112_emul_fail_writes_after(const struct emul *target, struct tac5112_reg reg,
				    uint8_t skip);

/**
 * @brief Assert the brown-out status right after the next read of a register.
 *
 * Lets a test raise an IRQ edge in the middle of the driver's fault collection
 * (for example after the output latches are read but before the live status).
 * One shot; removed by tac5112_emul_clear_failures().
 */
void tac5112_emul_brownout_after_read(const struct emul *target, struct tac5112_reg reg);

/**
 * @brief Assert the brown-out status just before the next read of a register
 * samples its value.
 *
 * Lets a test raise an IRQ edge between the driver deciding to read the live
 * status and the I2C read that samples it. One shot; shares its slot with
 * tac5112_emul_brownout_after_read().
 */
void tac5112_emul_brownout_before_read(const struct emul *target, struct tac5112_reg reg);

/** @brief Remove every read and write failure hook. */
void tac5112_emul_clear_failures(const struct emul *target);

/**
 * @brief Make the next @p count I2C reads of one register fail with -EIO.
 *
 * A failed read does not clear a latched register. Passing 0 cancels the hook.
 */
void tac5112_emul_fail_reads(const struct emul *target, struct tac5112_reg reg, uint8_t count);

/** @brief Read a latched fault register without clearing it. */
uint8_t tac5112_emul_latch_peek(const struct emul *target, struct tac5112_reg reg);

#ifdef __cplusplus
}
#endif

#endif /* TAC5112_EMUL_H_ */
