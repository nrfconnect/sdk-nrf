/* Generation of a random integer in a given range.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RNDINRANGE_HEADER_FILE
#define RNDINRANGE_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/** Create a task to generate a random integer in the interval [1, n[ where n is an odd integer
 * greater than 2.
 *
 * @param[in,out] t   Task structure to use.
 * @param[in] n       Upper limit, an unsigned integer in big endian form.
 * @param[in] nsz     Size in bytes of n.
 * @param[out] out    Address where the task writes the output random integer.
 *                    The storage area provided with this pointer must be in DMA memory and must
 *                    have a size of nsz bytes.
 *
 * The upper limit n must be odd and > 2. It must be provided in big endian form. It is allowed for
 * n to have zero bytes in the most significant positions. The size nsz must include any leading
 * zero bytes in n. If n has some zero bytes in the most significant positions, the task's output
 * will also have an equal number of leading zero bytes.
 *
 * The task produces a positive integer in big endian form, occupying exactly the same number of
 * bytes nsz as the input n. These output bytes are written starting from the out address. The
 * buffers pointed by n and out must not overlap.
 *
 * The task internally obtains random numbers by using the global PRNG by
 * calling cracen_get_random().
 * The user is responsible for making sure that the security strength of this PRNG is adequate for
 * his application. For instance, if the "rndinrange" task is used in the context of digital
 * signatures (e.g. generating keys, signing messages), the security strength requirements that must
 * be met by the underlying PRNG are specified in FIPS 186-4.
 */
void si_rndinrange_create(struct sitask *t, const unsigned char *n, size_t nsz, char *out);

#ifdef __cplusplus
}
#endif

#endif
