/** True Random Number Generator (TRNG).
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * The TRNG uses the hardware to generate random number with a high
 * level of entropy. It's intended to feed entropy into a DRBG.
 */

#ifndef TRNG_H
#define TRNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "trnginternal.h"

/** Force oscillators to run when the FIFO is full.
 *
 * This is a flag to be used for the 'control_bitmask' member of the TRNG
 * configuration structure. If this flag is set, the oscillators are forced to
 * run when the FIFO is full.
 *
 * Note: forcing the oscillators to run when the FIFO is full is intended for
 * TRNG hardware evaluation.
 */
#define SX_TRNG_OSCILLATORS_FORCE_RUN (1 << 11)

/** Bypass startup and continuous NIST health tests.
 *
 * This is a flag to be used for the 'control_bitmask' member of the TRNG
 * configuration structure. If this flag is set, the startup and continuous
 * NIST health tests are bypassed.
 *
 * Note: bypassing the NIST health tests is intended for TRNG hardware
 * evaluation and should not be done in production code.
 */
#define SX_TRNG_HEALTH_TEST_BYPASS (1 << 12)

/** Bypass startup and continuous AIS31 tests.
 *
 * This is a flag to be used for the 'control_bitmask' member of the TRNG
 * configuration structure. If this flag is set, the startup and continuous
 * AIS31 tests are bypassed. Only applicable if the hardware includes the AIS31
 * test module.
 *
 * Note: bypassing the AIS31 tests is intended for TRNG hardware evaluation
 * and should not be done in production code.
 */
#define SX_TRNG_AIS31_TEST_BYPASS (1 << 13)

/** Configuration parameters for the TRNG */
struct sx_trng_config {
	/** FIFO level below which the module leaves the idle state to refill
	 * the FIFO
	 *
	 * In numbers of 128-bit blocks.
	 *
	 * Set to 0 to use default (recommended).
	 */
	unsigned int wakeup_level;

	/** Number of clock cycles to wait before sampling data from the noise
	 * source
	 *
	 * Set to 0 to use default.
	 */
	unsigned int init_wait;

	/** Number of clock cycles to wait before stopping the rings after the
	 * FIFO is full.
	 *
	 * Set to 0 to use default.
	 */
	unsigned int off_time_delay;

	/** Clock divider for the frequency at which the outputs of the rings
	 * are sampled.
	 *
	 * Set to 0 to sample at APB interface clock frequency.
	 */
	unsigned int sample_clock_div;

	/** TRNG configuration bit mask.
	 *
	 * This bit mask can be formed by ORing together zero or more of these
	 * constants: SX_TRNG_OSCILLATORS_FORCE_RUN, SX_TRNG_HEALTH_TEST_BYPASS
	 * and SX_TRNG_AIS31_TEST_BYPASS.
	 */
	uint32_t control_bitmask;
};

/** TRNG initialization
 *
 * This function will do a soft reset of the TRNG hardware and will configure
 * it either with default configuration or with the user-provided \p config.
 *
 * This will power on CRACEN by calling @ref cracen_acquire.
 *
 * When the TRNG operation is finished sx_trng_close()
 * should be called.
 *
 * @param[in,out] ctx TRNG context to be used in other operations
 * @param[in] config pointer to optional configuration. NULL to use default.
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 */
int sx_trng_open(struct sx_trng *ctx, const struct sx_trng_config *config);

/** Get random bytes
 *
 * When this function returns ::SX_OK, \p size random bytes have been
 * written to the \p dst memory location. If not enough random bytes are
 * available, the function does not write any data to \p dst and returns
 * ::SX_ERR_HW_PROCESSING.
 *
 * If the hardware will never be able to provide the whole requested
 * size of random data, the function will return ::SX_ERR_TOO_BIG.
 * For more reliable operation, it's recommended to not request more than
 * 32 bytes of data with the default 'sx_trng_config::wakeup_level'.
 *
 * If ::SX_ERR_RESET_NEEDED error is returned, the previous context will have
 * to be closed with sx_trng_close() before sx_trng_open() is called
 * to obtain a new context.
 *
 * @param[in,out] ctx TRNG context to be used in other operations
 * @param[out] dst Destination in memory to copy \p size bytes to
 * @param[in] size length in bytes
 *
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_RESET_NEEDED
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - the sx_trng_open() must be called first.
 */
int sx_trng_get(struct sx_trng *ctx, char *dst, size_t size);

/**
 * @brief End TRNG context. Will power off Cracen.
 *
 * @param[in,out] ctx TRNG context.
 *
 * @return ::SX_OK
 */
int sx_trng_close(struct sx_trng *ctx);

#ifdef __cplusplus
}
#endif

#endif
