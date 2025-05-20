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

/** Blinding structure**/
struct sx_pk_blinder {
	union sx_pk_blinder_state {
		sx_pk_blind_factor blind; /**< Blind factor state of generation function */
		void *custom;		  /**< Custom state of generation function */
	} state;			  /**< Structure for state saving of generation function */
	sx_pk_blind_factor (*generate)(
		struct sx_pk_blinder *blinder); /**< Function pointer to a blinding generator */
};

/** Activate the blinding countermeasures where available
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] prng Pointer to blinder structure that holds the blinder
 * generation function
 */
void sx_pk_cnx_configure_blinding(struct sx_pk_cnx *cnx, struct sx_pk_blinder *prng);

/** Fills the blinder with default blind factor generation function
 *
 * @param[in,out] blinder Blinder to fill
 * @param[in] seed Non-zero random value for the default blind generation
 * function
 */
void sx_pk_default_blinder(struct sx_pk_blinder *blinder, sx_pk_blind_factor seed);

#ifdef __cplusplus
}
#endif

#endif
