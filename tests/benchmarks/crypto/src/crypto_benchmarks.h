/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRYPTO_BENCHMARKS_H__
#define CRYPTO_BENCHMARKS_H__

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#define APP_SUCCESS	    (0)
#define APP_ERROR	    (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE   "Example exited with error!"

/*
 * Message length every suite runs its algorithm over. One value for all, so
 * figures compare across algorithms; raising it costs RAM in every suite.
 *
 * Asserted at build time where it matters: the cipher suite needs a multiple of
 * the AES block for its no-padding modes, and the asymmetric encryption suite
 * no more than an RSA-2048 OAEP message carries.
 */
#define TEXT_SIZE 64

/*
 * Every multipart operation feeds its input in exactly two updates, so that
 * multipart rows compare across algorithms: a differing number of update calls
 * would move both figures for reasons unrelated to the algorithm. An operation
 * consuming a ciphertext rather than the message splits that in two instead.
 */
#define TEXT_HALF_SIZE (TEXT_SIZE / 2)

BUILD_ASSERT(TEXT_SIZE % 2 == 0, "TEXT_SIZE must be even to split into two updates");

/*
 * One operation: the body of one thread.
 *
 * fn must not log -- the thread's time and stack have to belong to the PSA
 * calls alone; check results in the suite's check instead. Returning
 * PSA_ERROR_NOT_SUPPORTED marks the operation skipped rather than failed.
 */
struct op {
	/** Last field of the thread name, for example "encrypt". */
	const char *name;
	psa_status_t (*fn)(const void *context);
	const void *context;
	/**
	 * Optional setup the row is not measuring: the far side of an exchange, a
	 * peer's key, a secret to compare against. Runs in its own thread, so its
	 * time and stack stay out of the operation's figures. An error fails the
	 * operation unmeasured; PSA_ERROR_NOT_SUPPORTED skips it.
	 */
	psa_status_t (*prepare)(const void *context);
};

/** One stage of a suite. A suite that leaves a stage out has none. */
struct stage {
	const struct op *ops;
	size_t op_cnt;
};

/**
 * Everything one algorithm demonstrates. An omitted stage does not apply to it:
 * SHA-256 has no key setup, RNG no multipart form.
 */
struct suite {
	/** First field of the thread name, for example "aes_cbc". */
	const char *alg;
	/** Optional second field of the thread name, for example "aes128". */
	const char *keydesc;
	/** Test data supplied to the suite's shared operations. */
	const void *context;

	struct stage keysetup;
	struct stage singlepart;
	struct stage multipart;

	/** Verifies what the operations produced; 0 on success. Runs on the runner. */
	int (*check)(void);
	void (*cleanup)(void);
};

/**
 * The suites of one src/suites/<name> folder, the family results are grouped by.
 * Named once per folder, so the grouping tracks the source tree.
 */
struct suite_group {
	/** Name of the folder under src/suites, for example "psa_cipher". */
	const char *name;
	const struct suite *const *suites;
	size_t suite_cnt;
};

/** Runs every stage of a suite, one thread per operation. */
void run_suite(const struct suite_group *group, const struct suite *suite);

/** Reports the per-operation results and timings, in the configured format. */
void report_summary(void);

/** Number of operations, and reports, that did not succeed. */
size_t failure_count(void);

/** Suite registry, in suites.c. */
extern const struct suite_group suite_groups[];
extern const size_t suite_group_cnt;

#endif /* CRYPTO_BENCHMARKS_H__ */
