/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PAKE_TEST_LOGIC_H__
#define PAKE_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

/*
 * How the PAKE suites are measured.
 *
 * Timing a whole exchange would time both sides at once and answer nothing, so
 * every suite measures the client only: two rows, the key it must hold before
 * starting, and the exchange through to its derived secret.
 *
 * The server stays out of those figures where the protocol allows. What it can
 * do before the client speaks runs in the prepare callback, which is neither
 * timed nor recorded; its last steps, which the client never waits for, run in
 * the suite's check. What is left is what the client cannot proceed without,
 * and each protocol's file names it.
 *
 * The client's operation is a local of the measured function, so its footprint
 * counts; everything of the server's is static, so none of it does.
 */

/** Bytes of the agreed secret each suite derives and compares. */
#define PAKE_SECRET_SIZE 32

/*
 * Parameters of one PAKE suite. The primitive and the KDF that turns the agreed
 * secret into bytes are choices of their own, made per suite to match the
 * standalone sample of each algorithm.
 */
struct pake_test_data {
	/** The PAKE algorithm, for example PSA_ALG_JPAKE(PSA_ALG_SHA_256). */
	psa_algorithm_t algorithm;
	/**
	 * KDF for the agreed key, never the PAKE algorithm itself:
	 * psa_key_derivation_setup() rejects those, and not every KDF accepts
	 * what a given PAKE hands out.
	 */
	psa_algorithm_t kdf;
	/** ECC curve family, or DH group family where the primitive is DH. */
	uint8_t family;
	/** Size of the primitive, in bits. */
	size_t bits;
};

/*
 * Who one side says it is. PSA_PAKE_ROLE_NONE skips psa_pake_set_role() and a
 * NULL peer skips psa_pake_set_peer(): J-PAKE has no roles, and SRP-6 no peer
 * identity -- the core rejects one, and the rejection aborts the operation.
 */
struct pake_side {
	psa_pake_role_t role;
	const char *user;
	const char *peer;
};

/** Imports the sample's password as a key object. Both sides do this for
 *  themselves, from the same password, as two real parties would.
 */
psa_status_t pake_import_password(psa_algorithm_t algorithm, psa_key_id_t *key);

/** Sets a side up: cipher suite, role and identities, in the order the core
 *  insists on.
 */
psa_status_t pake_setup_side(psa_pake_operation_t *operation,
			     const psa_pake_cipher_suite_t *suite, psa_key_id_t key,
			     const struct pake_side *side);

/*
 * The wire between the two sides. They never run together, so a message cannot
 * live on either one's stack; these move rounds through a store that outlives
 * both. A round written with pake_output_round() is read back step for step by
 * pake_input_round(), so what the store holds when an exchange returns is the
 * client's last message -- which is what a suite's check hands the server.
 */
psa_status_t pake_output_round(psa_pake_operation_t *operation, const psa_pake_step_t *steps,
			       size_t step_cnt);
psa_status_t pake_input_round(psa_pake_operation_t *operation, const psa_pake_step_t *steps,
			      size_t step_cnt);
psa_status_t pake_output_step(psa_pake_operation_t *operation, psa_pake_step_t step);
psa_status_t pake_input_step(psa_pake_operation_t *operation, psa_pake_step_t step);

/**
 * The secret one side ended up with, as PAKE_SECRET_SIZE comparable bytes. A
 * NULL info is for a KDF that takes the secret alone.
 */
psa_status_t pake_derive_secret(psa_pake_operation_t *operation, psa_algorithm_t kdf,
				const uint8_t *info, size_t info_length, uint8_t *secret);

/* Written out because the tables live in another translation unit, out of
 * ARRAY_SIZE's reach. Every protocol has the same two rows.
 */
#define PAKE_KEYSETUP_OP_COUNT 1
#define PAKE_EXCHANGE_OP_COUNT 1

extern const struct op ecjpake_keysetup_ops[];
extern const struct op ecjpake_exchange_ops[];
extern const struct op spake2p_keysetup_ops[];
extern const struct op spake2p_exchange_ops[];
extern const struct op srp6_keysetup_ops[];
extern const struct op srp6_exchange_ops[];

int ecjpake_check(void);
void ecjpake_cleanup(void);
int spake2p_check(void);
void spake2p_cleanup(void);
int srp6_check(void);
void srp6_cleanup(void);

/*
 * The exchange goes in the single-part stage; a PAKE has no single-part and
 * multipart forms to tell apart. Key setup gates it, so a key the platform will
 * not produce is reported once, not again as a failed exchange.
 */
#define PAKE_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, kdf_value, \
			 family_value, bits_value, protocol) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct pake_test_data){ \
			.algorithm = (algorithm_value), \
			.kdf = (kdf_value), \
			.family = (family_value), \
			.bits = (bits_value), \
		}, \
		.keysetup = {protocol##_keysetup_ops, PAKE_KEYSETUP_OP_COUNT}, \
		.singlepart = {protocol##_exchange_ops, PAKE_EXCHANGE_OP_COUNT}, \
		.check = protocol##_check, \
		.cleanup = protocol##_cleanup, \
	}

#define ECJPAKE_SUITE_ENTRY(key_description, hash_value, family_value, bits_value) \
	PAKE_SUITE_ENTRY("ecjpake", key_description, PSA_ALG_JPAKE(hash_value), \
			 PSA_ALG_TLS12_ECJPAKE_TO_PMS, family_value, bits_value, ecjpake)

#define SPAKE2P_SUITE_ENTRY(key_description, hash_value, family_value, bits_value) \
	PAKE_SUITE_ENTRY("spake2p", key_description, PSA_ALG_SPAKE2P_HMAC(hash_value), \
			 PSA_ALG_HKDF(PSA_ALG_SHA_256), family_value, bits_value, spake2p)

#define SRP6_SUITE_ENTRY(key_description, hash_value, family_value, bits_value) \
	PAKE_SUITE_ENTRY("srp6", key_description, PSA_ALG_SRP_6(hash_value), \
			 PSA_ALG_HKDF(PSA_ALG_SHA_256), family_value, bits_value, srp6)

#endif /* PAKE_TEST_LOGIC_H__ */
