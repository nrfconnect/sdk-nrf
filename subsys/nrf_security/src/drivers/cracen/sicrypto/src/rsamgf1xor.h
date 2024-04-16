/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RSAMGF1XOR_HEADER_FILE
#define RSAMGF1XOR_HEADER_FILE

/** Create a task to perform MGF1 and xor.
 *
 * MGF1 is the mask generation function defined in RFC 8017. This task computes
 * the mask and then applies it, by bitwise xor, to another input octet string.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      The hash algorithm on which MGF1 is based.
 *
 * After creating the task, si_task_consume() must be called to provide the MGF1
 * seed input. Then a call to si_task_produce() provides the desired mask length
 * and the address where the mask should be applied.
 *
 * This task needs a workmem buffer with size:
 *      hash_digest_size + 4
 * where all sizes are expressed in bytes.
 */
void si_create_mgf1xor(struct sitask *t, const struct sxhashalg *hashalg);

#endif
