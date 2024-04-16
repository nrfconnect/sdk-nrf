/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_ECC_HEADER_FILE
#define SICRYPTO_ECC_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sitask;
struct sx_pk_ecurve;

struct si_eccsk {
	const struct sx_pk_ecurve *curve;
	char *d;
};

struct si_eccpk {
	const struct sx_pk_ecurve *curve;
	char *qx;
	char *qy;
};

struct si_ecc_keypair {
	struct si_eccsk sk;
	struct si_eccpk pk;
};

struct sienv;

/** Create a task for ECC public key computation.
 *
 * @param[in,out] t     Task structure to use.
 * @param[in] sk        ECC private key.
 * @param[out] pk       Output ECC public key.
 *
 * This task does not need a workmem buffer.
 */
void si_ecc_create_genpubkey(struct sitask *t, const struct si_eccsk *sk, struct si_eccpk *pk);

/** Create a task for ECC private key generation.
 *
 * @param[in,out] t     Task structure to use.
 * @param[in] curve     Curve used by the key.
 * @param[out] sk       Output ECC private key.
 *
 * The PRNG in the sicrypto environment must have been set up prior to calling
 * this function.
 *
 * This task needs a workmem buffer with size at least equal to the key size.
 */
void si_ecc_create_genprivkey(struct sitask *t, const struct sx_pk_ecurve *curve,
			      struct si_eccsk *sk);

#ifdef __cplusplus
}
#endif

#endif
