/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mfg_log.h
 *
 * Manufacturing application logging macros.
 *
 * Two output modes are supported, selected at build time via Kconfig:
 *
 * Human-readable (default, CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE=n):
 *   Verbose, multi-line output intended for operators reviewing serial logs.
 *   MFG_LOG_STEP includes the source file and line number so operators can
 *   locate the corresponding source code.
 *
 *   NOTE: Call MFG_LOG_STEP exactly once at the top of each step function so
 *   that __FILE__ and __LINE__ expand to the correct location.
 *
 * Machine-readable (CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE=y):
 *   Minimal, line-oriented output intended for automated test equipment.
 *   Each line is prefixed with a type tag:
 *
 *     STEP:<name>        — a manufacturing step is starting
 *     ERR:<message>      — an error condition was detected
 *     KV:<key>:<value>   — structured datum (LCS state, IAK pubkey, ...)
 *     DONE:PASS          — manufacturing completed successfully
 *     DONE:FAIL          — manufacturing halted with an error
 *
 *   MFG_LOG_INF lines are suppressed; they carry verbose human-readable
 *   explanations that automated parsers do not need.
 *
 */

#ifndef MFG_LOG_H_
#define MFG_LOG_H_

#include <zephyr/sys/printk.h>
#include <stdint.h>
#include <stddef.h>

#if defined(CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE)

/** Step entry — emits "STEP:<name>". */
#define MFG_LOG_STEP(msg)       printk("STEP:" msg "\n")

/** Informational — suppressed in machine-readable mode. */
#define MFG_LOG_INF(...)        /* suppressed */

/** Error — emits "ERR:<formatted message>". */
#define MFG_LOG_ERR(...)        do { printk("ERR:"); printk(__VA_ARGS__); } while (0)

/**
 * Structured key-value datum — emits "KV:<key>:<value>".
 * @p key   must be a string literal.
 * @p value is a string expression (literal or runtime pointer).
 */
#define MFG_LOG_KV(key, value)  printk("KV:" key ":%s\n", (value))

/** Final manufacturing result — emits "DONE:PASS" or "DONE:FAIL". */
#define MFG_LOG_DONE(ok)        printk("DONE:%s\n", (ok) ? "PASS" : "FAIL")

#else /* human-readable (default) */

/** Step entry — emits step description with source location. */
#define MFG_LOG_STEP(msg)       printk("\n" msg ", " __FILE__ ", line: %d\n", __LINE__)

/** Plain informational log line. */
#define MFG_LOG_INF(...)        printk(__VA_ARGS__)

/** Error log line. */
#define MFG_LOG_ERR(...)        printk(__VA_ARGS__)

/**
 * Structured key-value datum — emits "<key>: <value>".
 * @p key   must be a string literal.
 * @p value is a string expression (literal or runtime pointer).
 */
#define MFG_LOG_KV(key, value)  printk(key ": %s\n", (value))

/** Final manufacturing result — printed as a summary line. */
#define MFG_LOG_DONE(ok)        printk("\nManufacturing %s.\n", (ok) ? "complete. PASS" : "FAILED")

#endif /* CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE */

/**
 * Emit a binary buffer as hex, structured for the active log mode.
 *
 * Human-readable: multi-line hex dump, 16 bytes per row, indented.
 * Machine-readable: single "KV:<key>:<hex>" line.
 *
 * @param key   Identifier string for the datum (e.g. "iak_pubkey").
 * @param data  Pointer to byte buffer.
 * @param len   Number of bytes to emit.
 */
static inline void mfg_log_bytes(const char *key, const uint8_t *data, size_t len)
{
#if defined(CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE)
	printk("KV:%s:", key);
	for (size_t i = 0; i < len; i++) {
		printk("%02x", data[i]);
	}
	printk("\n");
#else
	printk("  %s (%zu bytes):\n  ", key, len);
	for (size_t i = 0; i < len; i++) {
		printk("%02x", data[i]);
		if ((i + 1) % 16 == 0 && (i + 1) < len) {
			printk("\n  ");
		}
	}
	printk("\n");
#endif
}

#endif /* MFG_LOG_H_ */
