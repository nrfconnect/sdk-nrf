/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_MEM_HEADER_FILE
#define SICRYPTO_MEM_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/** Compare two data buffers for equality.
 *
 * @param[in] a         Input data buffer.
 * @param[in] b         Input data buffer.
 * @param[in] sz        Size in bytes of the input data buffers.
 * @return              0 if and only if \p a and \p b are equal or \p sz is 0.
 *                      A value != 0 is returned if \p a and \p b are different.
 */
int si_memdiff(const char *a, const char *b, size_t sz);

#ifdef __cplusplus
}
#endif

#endif
