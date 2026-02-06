/** Blinding header file
 *
 * @file
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BLINDING_HEADER_FILE
#define BLINDING_HEADER_FILE

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cnx;

/** Blinding factor used for Counter Measures */
typedef uint64_t sx_pk_blind_factor;

/** Get a blinding factor needed for the ECC countermeasures.
 *
 * @param[in,out] bld_factor Blinding factor to be generated
 */
int sx_pk_get_blinding_factor(sx_pk_blind_factor *bld_factor);
#ifdef __cplusplus
}
#endif

#endif
