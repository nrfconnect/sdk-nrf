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
 * @param target Emulator instance.
 * @param reg    One of TAC5112_REG_INT_LTCH0, TAC5112_REG_OUT_CH1_LTCH or
 *               TAC5112_REG_OUT_CH2_LTCH.
 * @param bits   Latch bits to set.
 *
 * @return 0 if successful, error otherwise.
 */
int tac5112_emul_inject_latch(const struct emul *target, struct tac5112_reg reg, uint8_t bits);

/**
 * @brief Set or clear the brown-out shutdown status (AVDD_IOVDD_STS).
 *
 * This is a live status bit: it is not cleared by reading and stays set until
 * the test clears it with @c active = false.
 *
 * @param target  Emulator instance.
 * @param active  Live state of the target status bit.
 */
void tac5112_emul_set_brownout(const struct emul *target, bool active);

/**
 * @brief Make I2C writes to one register fail with -EIO (or stop doing so).
 *
 * Lets tests exercise the driver's error paths, including partial failure of
 * multi-register updates. Writes to other registers are unaffected.
 *
 * @param target  Emulator instance.
 * @param reg     Register to fail write.
 * @param fail    Flag to enable fail.
 */
void tac5112_emul_fail_writes(const struct emul *target, struct tac5112_reg reg, bool fail);

/**
 * @brief Let @p skip writes to a register succeed, then fail all later ones.
 *
 * Allows a sequence such as "first write succeeds, rollback write fails".
 * At most two registers can have a write-failure hook at once.
 *
 * @param target  Emulator instance.
 * @param reg     Register to skip write.
 * @param skip    Number of iterations to skip write of register.
 */
void tac5112_emul_fail_writes_after(const struct emul *target, struct tac5112_reg reg,
				    uint8_t skip);

/**
 * @brief Assert the brown-out status right after the next read of a register.
 *
 * Lets a test raise an IRQ edge in the middle of the driver's fault collection
 * (for example after the output latches are read but before the live status).
 * One shot; removed by tac5112_emul_clear_failures().
 *
 * @param target  Emulator instance.
 * @param reg     Register to read.
 */
void tac5112_emul_brownout_after_read(const struct emul *target, struct tac5112_reg reg);

/**
 * @brief Assert the brown-out status just before the next read of a register
 * samples its value.
 *
 * Lets a test raise an IRQ edge between the driver deciding to read the live
 * status and the I2C read that samples it. One shot; shares its slot with
 * tac5112_emul_brownout_after_read().
 *
 * @param target  Emulator instance.
 * @param reg     Register to read.
 */
void tac5112_emul_brownout_before_read(const struct emul *target, struct tac5112_reg reg);

/**
 * @brief End the brown-out and latch an output fault right after the next read
 * of a register returns its value.
 *
 * Models a brown-out that is sampled active and then clears while an output
 * fault asserts a new IRQ edge, before the driver acts on the sample. One
 * shot; shares its slot with tac5112_emul_brownout_after_read().
 *
 * @param target     Emulator instance.
 * @param reg        Register whose read triggers the sequence.
 * @param fault_reg  Latched fault register to set.
 * @param bits       Latch bits to set.
 */
void tac5112_emul_clear_brownout_and_fault_after_read(const struct emul *target,
						      struct tac5112_reg reg,
						      struct tac5112_reg fault_reg, uint8_t bits);

/**
 * @brief Clear and reassert the brown-out status right after the next read of
 * a register returns its value.
 *
 * Models a brown-out that ends and returns after the driver sampled the status
 * but before it acted on the sample. One shot; shares its slot with
 * tac5112_emul_brownout_after_read().
 *
 * @param target  Emulator instance.
 * @param reg     Register to read.
 */
void tac5112_emul_brownout_cycle_after_read(const struct emul *target, struct tac5112_reg reg);

/**
 * @brief Clear and reassert the brown-out status just before the next read of
 * a register samples its value.
 *
 * Models a brown-out that ends and returns while the driver is collecting
 * faults: the IRQ line drops and rises again, and the sample shows the status
 * set. One shot; shares its slot with tac5112_emul_brownout_after_read().
 *
 * @param target  Emulator instance.
 * @param reg     Register to read.
 */
void tac5112_emul_brownout_cycle_before_read(const struct emul *target, struct tac5112_reg reg);

/** @brief Remove every read and write failure hook.
 *
 * @param target  Emulator instance.
 */
void tac5112_emul_clear_failures(const struct emul *target);

/**
 * @brief Make the next @p count I2C reads of one register fail with -EIO.
 *
 * A failed read does not clear a latched register. Passing 0 cancels the hook.
 *
 * @param target  Emulator instance.
 * @param reg     Register to read.
 * @param count   Number of reads to fail.
 */
void tac5112_emul_fail_reads(const struct emul *target, struct tac5112_reg reg, uint8_t count);

/** @brief Read a latched fault register without clearing it.
 *
 * @param target  Emulator instance.
 * @param reg     Register to read.
 */
uint8_t tac5112_emul_latch_peek(const struct emul *target, struct tac5112_reg reg);

#ifdef __cplusplus
}
#endif

#endif /* TAC5112_EMUL_H_ */
