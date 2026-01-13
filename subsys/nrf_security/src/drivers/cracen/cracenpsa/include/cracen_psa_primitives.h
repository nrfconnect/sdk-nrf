/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_primitives
 * @{
 * @brief Primitive operations and data structures for the CRACEN PSA driver.
 *
 * @note These primitives are shared between the public driver API and internal
 *       implementation.
 *
 * This module provides primitive data structures and constants, shared between
 * the public driver API and internal implementation.
 *
 * @warning The structs in this module contain sensitive data. If used directly,
 *          you must make sure that they are correctly wiped after usage
 *          to avoid leaking sensitive data.
 *
 * Unless otherwise specified, all size and length defines are in bytes.
 */

#ifndef CRACEN_PSA_PRIMITIVES_H
#define CRACEN_PSA_PRIMITIVES_H

#include <psa/crypto_sizes.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>
#include <silexpk/blinding.h>
#include <stdbool.h>
#include <stdint.h>
#include <sxsymcrypt/cmac.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/trng.h>
#include <sxsymcrypt/hashdefs.h>

#if defined(PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
#if defined(PSA_NEED_CRACEN_CHACHA20_POLY1305)
#include "poly1305_ext.h"
#endif /* PSA_NEED_CRACEN_CHACHA20_POLY1305 */
#endif /* PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS */

#define SX_BLKCIPHER_IV_SZ	(16U)
#define SX_BLKCIPHER_AES_BLK_SZ (16U)

#define SX_BLKCIPHER_CHACHA20_BLK_SZ (64U)

#if defined(PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20)
/** Maximum block cipher block size.
 *
 * ChaCha20 has a 512-bit block size.
 */
#define SX_BLKCIPHER_MAX_BLK_SZ SX_BLKCIPHER_CHACHA20_BLK_SZ
#else
#define SX_BLKCIPHER_MAX_BLK_SZ SX_BLKCIPHER_AES_BLK_SZ
#endif

#define CRACEN_MAX_AES_KEY_SIZE (32u)
#define CRACEN_MAX_AEAD_BLOCK_SIZE (64u)

/** Maximum AEAD key size.
 *
 * CRACEN AEAD uses two backends, BA411 for GCM/CCM and BA417 for
 * ChaChaPoly. Both have 32 bytes as the max key size.
 */
#define CRACEN_MAX_AEAD_KEY_SIZE   (32u)

/** Maximum ChaCha20 key size.
 *
 * The low level driver only supports a key size of exactly 32 bytes for
 * CHACHA20 (and for CHACHAPOLY).
 */
#define CRACEN_MAX_CHACHA20_KEY_SIZE (32u)

/* Maximum ChaCha20 counter size. */
#define CRACEN_CHACHA20_COUNTER_SIZE (4u)

/* Defined by RFC8439 */
#define CRACEN_POLY1305_TAG_SIZE (16u)
#define CRACEN_POLY1305_KEY_SIZE (32u)
/* BA417 uses 131 bit */
#define CRACEN_POLY1305_ACC_SIZE (17u)

/** Maximum cipher key size.
 *
 * There are two key types supported for ciphers, CHACHA20 and AES,
 * and they both have a max key size of 32.
 */
#define CRACEN_MAX_CIPHER_KEY_SIZE (32u)

/** Maximum length of PBKDF2 salt.
 *
 * There is no set max length. For this implementation max length is set
 * to 128 bytes.
 */
#define CRACEN_PBKDF_MAX_SALT_SIZE 128

/** Maximum length for HKDF info input.
 *
 * The input is optional and has no set max length. For this implementation
 * max length is set to 128 bytes.
 */
#define CRACEN_HKDF_MAX_INFO_SIZE 128

#define CRACEN_MAC_MAX_LABEL_SIZE 127
#define CRACEN_MAC_MAX_CONTEXT_SIZE 64
#define CRACEN_JPAKE_USER_ID_MAX_SIZE 16
#define CRACEN_P256_KEY_SIZE   32
#define CRACEN_P256_POINT_SIZE 64
#define CRACEN_TLS12_PRF_MAX_LABEL_SIZE 128
#define CRACEN_TLS12_PRF_MAX_SEED_SIZE	128

/** PRNG key size.
 *
 * Key size to get 256 bits of security.
 */
#define CRACEN_PRNG_KEY_SIZE		     (32u)

/** PRNG entropy size.
 *
 * Seed equals key length plus 16 bytes.
 */
#define CRACEN_PRNG_ENTROPY_SIZE	     (48u)

/** PRNG nonce size.
 *
 * Not supported for CTR_DRBG.
 */
#define CRACEN_PRNG_NONCE_SIZE		     (0u)

/** Maximum PRNG personalization size.
 *
 * Maximum equals the seed size.
 */
#define CRACEN_PRNG_PERSONALIZATION_MAX_SIZE (48u)

/** PRNG working memory size.
 *
 * Key length plus AES block length plus XORBUF_SZ (128).
 */
#define CRACEN_PRNG_WORKMEM_SIZE	     (176u)

/** Maximum PRNG request size.
 *
 * Maximum request size. See NIST specification.
 */
#define CRACEN_PRNG_MAX_REQUEST_SIZE	     (1u << 16)

#define CRACEN_SPAKE2P_HASH_LEN PSA_HASH_LENGTH(PSA_ALG_SHA_256)

#define CRACEN_WPA3_SAE_MAX_SSID_LEN	(32u)
#define CRACEN_WPA3_SAE_MAX_PWD_LEN	(256u)
#define CRACEN_WPA3_SAE_MAX_PWID_LEN	(256u)

/** WPA3 SAE PMK (Pairwise Master Key) length.
 *
 * According to IEEE802.11-2024, 12.7.1.3.
 */
#define CRACEN_WPA3_SAE_PMK_LEN		(32u)

#define CRACEN_WPA3_SAE_PMKID_SIZE	(16u)
#define CRACEN_WPA3_SAE_STA_ID_LEN	(6u)

/** WPA3 SAE IANA group size.
 *
 * IANA group to identify the elliptic curve.
 * See IEEE802.11-2024, Table 12-2.
 */
#define CRACEN_WPA3_SAE_IANA_GROUP_SIZE	(2u)

/** WPA3 SAE commit message size.
 *
 * Size includes P-256 key size, and P-256 point size.
 */
#define CRACEN_WPA3_SAE_COMMIT_SIZE	(CRACEN_P256_KEY_SIZE		 + \
					 CRACEN_P256_POINT_SIZE)

#define CRACEN_WPA3_SAE_SEND_CONFIRM_SIZE	(2u)

/** WPA3 SAE confirm message size.
 *
 * Size includes send-confirm counter size plus hash length.
 */
#define CRACEN_WPA3_SAE_CONFIRM_SIZE		(CRACEN_WPA3_SAE_SEND_CONFIRM_SIZE + \
						 PSA_HASH_LENGTH(PSA_ALG_SHA_256))

/** 4-bit Shoup's table.
 *  The size is defined as 2^4.
 */
#define CRACEN_AES_GCM_HTABLE_SIZE 16

enum cipher_operation {
	CRACEN_DECRYPT,
	CRACEN_ENCRYPT
};

/** Key derivation state machine states. */
enum cracen_kd_state {
	CRACEN_KD_STATE_INVALID = 0,

	/* HKDF states: */
	CRACEN_KD_STATE_HKDF_INIT = 0x10,
	CRACEN_KD_STATE_HKDF_STARTED,
	CRACEN_KD_STATE_HKDF_KEYED,
	CRACEN_KD_STATE_HKDF_OUTPUT,

	/* PBKDF2 states: */
	CRACEN_KD_STATE_PBKDF2_INIT = 0x20,
	CRACEN_KD_STATE_PBKDF2_SALT,
	CRACEN_KD_STATE_PBKDF2_PASSWORD,
	CRACEN_KD_STATE_PBKDF2_OUTPUT,

	/* CMAC/HMAC CTR states: */
	CRACEN_KD_STATE_MAC_CTR_INIT = 0x40,
	CRACEN_KD_STATE_MAC_CTR_KEY_LOADED,
	CRACEN_KD_STATE_MAC_CTR_INPUT_LABEL,
	CRACEN_KD_STATE_MAC_CTR_INPUT_CONTEXT,
	CRACEN_KD_STATE_MAC_CTR_OUTPUT,

	/* TLS12 PRF states: */
	CRACEN_KD_STATE_TLS12_PRF_INIT = 0x80,
	CRACEN_KD_STATE_TLS12_PRF_OUTPUT,

	/* TLS12 PSK TO MS states: */
	CRACEN_KD_STATE_TLS12_PSK_TO_MS_INIT = 0x100,
	CRACEN_KD_STATE_TLS12_PSK_TO_MS_OUTPUT,

	/* TLS12 EC J-PAKE to PMS state: */
	CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_INIT = 0x200,
	CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_OUTPUT,

	/* WPA3-SAE H2E states: */
	CRACEN_KD_STATE_WPA3_SAE_H2E_INIT = 0x400,
	CRACEN_KD_STATE_WPA3_SAE_H2E_SALT,
	CRACEN_KD_STATE_WPA3_SAE_H2E_PASSWORD,
	CRACEN_KD_STATE_WPA3_SAE_H2E_INFO,
	CRACEN_KD_STATE_WPA3_SAE_H2E_OUTPUT,
};

/** CRACEN context state.
 *
 * States to keep track of when the AEAD sxsymcrypt context has been
 * initialized with xxx_create and when the hardware is reserved with a
 * xxx_resume call.
 */
enum cracen_context_state {
	CRACEN_NOT_INITIALIZED = 0x0,
	CRACEN_CONTEXT_INITIALIZED = 0x1,
	CRACEN_HW_RESERVED = 0x2
};

/** Hash operation context. */
struct cracen_hash_operation_s {
	const struct sxhashalg *sx_hash_algo;

	/* Internal sxcrypto context. */
	struct sxhash sx_ctx;

	/* The driver can perform a processing round after getting a multiple of the block size.
	 * Therefore, the driver must know how much data is left to fill the next block.
	 */
	size_t bytes_left_for_next_block;

	/* Buffer for input data to fill up the next block. */
	uint8_t input_buffer[SX_HASH_MAX_ENABLED_BLOCK_SIZE];

	/* Flag indicating saved state exists that needs to be resumed */
	bool has_saved_state;
};
typedef struct cracen_hash_operation_s cracen_hash_operation_t;

/** Cipher operation context. */
struct cracen_cipher_operation {
	psa_algorithm_t alg;
	struct sxblkcipher cipher;
	bool initialized;
	uint8_t iv[SX_BLKCIPHER_IV_SZ];
	struct sxkeyref keyref;
	uint8_t key_buffer[CRACEN_MAX_CIPHER_KEY_SIZE];
	uint8_t unprocessed_input[SX_BLKCIPHER_MAX_BLK_SZ];
	uint8_t unprocessed_input_bytes;
	uint8_t blk_size;
	enum cipher_operation dir;
};
typedef struct cracen_cipher_operation cracen_cipher_operation_t;

/** Software CCM context for CRACEN software implementations. */
struct cracen_sw_ccm_context_s {
	uint8_t cbc_mac[SX_BLKCIPHER_AES_BLK_SZ]; /* CBC-MAC state. */
	uint8_t ctr_block[SX_BLKCIPHER_AES_BLK_SZ]; /* Counter block for CTR mode. */
	uint8_t keystream[SX_BLKCIPHER_AES_BLK_SZ]; /* Generated keystream. */
	uint8_t partial_block[SX_BLKCIPHER_AES_BLK_SZ]; /* Buffer for partial blocks. */
	size_t keystream_offset; /* Position in keystream buffer. */
	size_t total_ad_fed; /* Total AD bytes processed. */
	size_t data_partial_len; /* Partial data block length. */
	bool cbc_mac_initialized; /* CBC-MAC initialization flag. */
	bool ctr_initialized; /* CTR initialization flag. */
	bool has_partial_ad_block; /* Partial AD block flag. */
};
typedef struct cracen_sw_ccm_context_s cracen_sw_ccm_context_t;

/** Software GCM context for CRACEN software implementations. */
struct cracen_sw_gcm_context_s {
	/* Precalculated HTable */
	uint64_t h_table[CRACEN_AES_GCM_HTABLE_SIZE][SX_BLKCIPHER_AES_BLK_SZ / sizeof(uint64_t)];
	uint8_t ghash_block[SX_BLKCIPHER_AES_BLK_SZ]; /* GHASH calculation result */
	uint8_t ctr_block[SX_BLKCIPHER_AES_BLK_SZ]; /* Counter block for CTR mode */
	uint8_t keystream[SX_BLKCIPHER_AES_BLK_SZ]; /* Generated keystream */
	size_t keystream_offset; /* Position in keystream buffer */
	size_t total_ad_fed; /* Total AD bytes processed */
	size_t total_data_enc; /* Total size of the ciphertext */
	bool ctr_initialized; /* CTR initialization flag */
	bool ghash_initialized; /* GHASH initialization flag */
};
typedef struct cracen_sw_gcm_context_s cracen_sw_gcm_context_t;

#if defined(PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
#if defined(PSA_NEED_CRACEN_CHACHA20_POLY1305)
/** Software ChaCha20-Poly1305 context for CRACEN software implementations. */
struct cracen_sw_chacha20_poly1305_context_s {
	uint8_t ctr[CRACEN_CHACHA20_COUNTER_SIZE]; /* Counter */
	uint8_t keystream[SX_BLKCIPHER_CHACHA20_BLK_SZ]; /* Generated keystream */
	size_t keystream_offset; /* Position in keystream buffer */
	poly1305_ext_context poly_ctx;
	size_t total_ad_fed; /* Total AD bytes processed */
	size_t total_data_enc; /* Total size of the ciphertext */
	bool ctr_initialized; /* CTR initialization flag */
	bool poly_initialized; /* Poly1305 initialization flag */
};
typedef struct cracen_sw_chacha20_poly1305_context_s cracen_sw_chacha20_poly1305_context_t;
#endif /* PSA_NEED_CRACEN_CHACHA20_POLY1305 */
#endif /* PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS */

/** AEAD operation context for CRACEN software implementations. */
struct cracen_aead_operation {
	psa_algorithm_t alg;
	struct sxkeyref keyref;
	uint8_t key_buffer[CRACEN_MAX_AEAD_KEY_SIZE];
	uint8_t nonce[PSA_AEAD_NONCE_MAX_SIZE];
	uint8_t nonce_length;
	size_t ad_length;
	size_t plaintext_length;
	enum cipher_operation dir;
	uint8_t unprocessed_input[CRACEN_MAX_AEAD_BLOCK_SIZE];
	uint8_t unprocessed_input_bytes;
	uint8_t tag_size;
	enum cracen_context_state context_state;
	bool ad_finished;
	struct sxaead ctx;
#if defined(PSA_NEED_CRACEN_CTR_SIZE_WORKAROUNDS)
#if defined(PSA_NEED_CRACEN_CCM_AES)
	cracen_sw_ccm_context_t sw_ccm_ctx;
#endif /* PSA_NEED_CRACEN_CCM_AES */
#endif /* PSA_NEED_CRACEN_CTR_SIZE_WORKAROUNDS */

#if defined(PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
#if defined(PSA_NEED_CRACEN_GCM_AES)
	cracen_sw_gcm_context_t sw_gcm_ctx;
#endif /* PSA_NEED_CRACEN_GCM_AES */

#if defined(PSA_NEED_CRACEN_CHACHA20_POLY1305)
	cracen_sw_chacha20_poly1305_context_t sw_chacha_poly_ctx;
#endif /* PSA_NEED_CRACEN_CHACHA20_POLY1305 */
#endif /* PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS */
};
typedef struct cracen_aead_operation cracen_aead_operation_t;

/** CMAC context for multipart workarounds. */
struct cracen_cmac_context_s {
	uint8_t mac_state[SX_BLKCIPHER_AES_BLK_SZ]; /* Current MAC state. */
	uint8_t partial_block[SX_BLKCIPHER_AES_BLK_SZ];
	size_t partial_len;
	size_t processed_len; /* Total length of data processed so far, in bytes. */
};
typedef struct cracen_cmac_context_s cracen_cmac_context_t;

/** MAC operation context. */
struct cracen_mac_operation_s {
	psa_algorithm_t alg;
	size_t mac_size;

	/* The driver can perform a processing round after getting a multiple of the block size.
	 * Therefore, the driver must know how much data is left to fill the next block.
	 */
	size_t bytes_left_for_next_block;

	/* Buffer for input data to fill up the next block. */
	uint8_t input_buffer[SX_MAX(SX_HASH_MAX_ENABLED_BLOCK_SIZE, SX_BLKCIPHER_PRIV_SZ)];

	/* Flag indicating saved state exists that needs to be resumed */
	bool has_saved_state;
	union {
#if defined(PSA_NEED_CRACEN_HMAC)
		struct {
			struct sxhash hashctx;
			uint8_t workmem[SX_HASH_MAX_ENABLED_BLOCK_SIZE +
					PSA_HASH_MAX_SIZE];
		} hmac;
#endif /* PSA_NEED_CRACEN_HMAC */
#if defined(PSA_NEED_CRACEN_CMAC)
		struct {
			struct sxmac ctx;
			struct sxkeyref keyref;
			uint8_t key_buffer[CRACEN_MAX_AES_KEY_SIZE];
#if defined(PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
			cracen_cmac_context_t sw_ctx;
			struct sxblkcipher cipher;
#endif /* PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS */
		} cmac;
#endif /* PSA_NEED_CRACEN_CMAC */
		uint8_t _unused;
	};
};
typedef struct cracen_mac_operation_s cracen_mac_operation_t;

/** Key derivation operation context. */
struct cracen_key_derivation_operation {
	psa_algorithm_t alg;
	enum cracen_kd_state state;
	uint64_t capacity;
	uint8_t output_block[SX_MAX(SX_HASH_MAX_ENABLED_BLOCK_SIZE, SX_BLKCIPHER_PRIV_SZ)];
	uint8_t output_block_available_bytes;
	union{
		cracen_mac_operation_t mac_op;
		cracen_hash_operation_t hash_op;
	};
	union {
#if defined(PSA_NEED_CRACEN_HKDF)
		struct {
			uint8_t blk_counter;
			uint8_t prk[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
			uint8_t t[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
			uint8_t info[CRACEN_HKDF_MAX_INFO_SIZE];
			size_t info_length;
			bool info_set;
		} hkdf;
#endif /* PSA_NEED_CRACEN_HKDF */
#if defined(PSA_NEED_CRACEN_PBKDF2_HMAC)
		struct {
			uint64_t input_cost;
			uint8_t password[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
			size_t password_length;
			uint8_t salt[CRACEN_PBKDF_MAX_SALT_SIZE];
			size_t salt_length;
			uint32_t blk_counter;
			uint8_t uj[PSA_MAC_MAX_SIZE];
			uint8_t tj[PSA_MAC_MAX_SIZE];
		} pbkdf2;
#endif /* PSA_NEED_CRACEN_PBKDF2_HMAC */
#if	defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC) || \
	defined(PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
		struct {
#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
			psa_key_lifetime_t key_lifetime;
			mbedtls_svc_key_id_t key_id;
			uint8_t key_buffer[CRACEN_MAX_AES_KEY_SIZE];
			size_t key_size;
			uint8_t K_0[SX_BLKCIPHER_AES_BLK_SZ];
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */
			uint32_t counter;
			/* The +1 here is meant to store an algorithm-specific byte needed after
			 * the label.
			 */
			uint8_t label[CRACEN_MAC_MAX_LABEL_SIZE + 1];
			size_t label_length;
			uint8_t context[CRACEN_MAC_MAX_CONTEXT_SIZE];
			size_t context_length;
			uint32_t L;
		} mac_ctr;
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC || PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC */
#if defined(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)
		struct {
			uint8_t key[32];
		} ecjpake_to_pms;
#endif /* PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS */
#if defined(PSA_NEED_CRACEN_TLS12_PRF) || defined(PSA_NEED_CRACEN_TLS12_PSK_TO_MS)
		struct {
			/* May contain secret, length of secret as uint16be, other secret and
			 * other secret length as uint16be.
			 */
			uint8_t secret[PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE * 2 + 4];
			size_t secret_length;
			uint8_t seed[CRACEN_TLS12_PRF_MAX_SEED_SIZE];
			size_t seed_length;
			uint8_t label[CRACEN_TLS12_PRF_MAX_LABEL_SIZE];
			size_t label_length;
			size_t counter;
			uint8_t a[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
		} tls12;
#endif /* PSA_NEED_CRACEN_TLS12_PRF || PSA_NEED_CRACEN_TLS12_PSK_TO_MS */
	};
};
typedef struct cracen_key_derivation_operation cracen_key_derivation_operation_t;

/** JPAKE operation context. */
struct cracen_jpake_operation {
	psa_algorithm_t alg; /* Hashing algorithm. */
	uint8_t secret[CRACEN_P256_KEY_SIZE]; /* Secret value derived from the password. */
	uint8_t user_id[CRACEN_JPAKE_USER_ID_MAX_SIZE];
	uint8_t peer_id[CRACEN_JPAKE_USER_ID_MAX_SIZE];
	size_t user_id_length;
	size_t peer_id_length;
	uint8_t rd_idx;
	uint8_t wr_idx;
	uint8_t x[3][CRACEN_P256_KEY_SIZE]; /* Private keys. */
	uint8_t X[3][CRACEN_P256_POINT_SIZE]; /* Public keys. */
	uint8_t P[3][CRACEN_P256_POINT_SIZE]; /* Peer keys. */
	uint8_t V[CRACEN_P256_POINT_SIZE];
	uint8_t r[CRACEN_P256_KEY_SIZE];
	const struct sx_pk_ecurve *curve;
};
typedef struct cracen_jpake_operation cracen_jpake_operation_t;

/** PRNG context. */
struct cracen_prng_context {
	uint8_t key[CRACEN_PRNG_KEY_SIZE];
	uint8_t V[SX_BLKCIPHER_AES_BLK_SZ];
	uint64_t reseed_counter;
	uint32_t initialized;
};
typedef struct cracen_prng_context cracen_prng_context_t;

#define CRACEN_SRP_HASH_ALG		 PSA_ALG_SHA_512
#define CRACEN_SRP_HASH_LENGTH		 PSA_HASH_LENGTH(CRACEN_SRP_HASH_ALG)
/** SRP RFC3526 key size in bits. */
#define CRACEN_SRP_RFC3526_KEY_BITS_SIZE 3072
#define CRACEN_SRP_FIELD_SIZE		 PSA_BITS_TO_BYTES(CRACEN_SRP_RFC3526_KEY_BITS_SIZE)

/** Maximum SRP salt length.
 *
 * The same number that Oberon PSA driver uses to align the implementations.
 */
#define CRACEN_SRP_MAX_SALT_LENGTH	 64

/** SRP operation context. */
struct cracen_srp_operation {
	psa_pake_role_t role; /* Protocol role. */
	uint8_t user_hash[CRACEN_SRP_HASH_LENGTH]; /* Hash of the username. */
	uint8_t salt[CRACEN_SRP_MAX_SALT_LENGTH]; /* Salt chosen by server. */
	size_t salt_len; /* Length of salt. */
	union {
		/* x = SHA512(salt | SHA512(user | ":" | pass)). */
		uint8_t x[CRACEN_SRP_HASH_LENGTH];
		uint8_t v[CRACEN_SRP_FIELD_SIZE]; /* v = g^x mod N. */
	};
	uint8_t ab[32]; /* Random a for client, b for server. */
	uint8_t A[CRACEN_SRP_FIELD_SIZE]; /* (v + g^b) mod N. */
	uint8_t B[CRACEN_SRP_FIELD_SIZE]; /* g^a mod N. */

	uint8_t M[CRACEN_SRP_HASH_LENGTH]; /* H(H(N) xor H(g) | H(U) | salt | A | B | K). */
	uint8_t K[CRACEN_SRP_HASH_LENGTH]; /* Shared key k = H_Interleaved(S). */
};
typedef struct cracen_srp_operation cracen_srp_operation_t;

/** SPAKE2+ operation context. */
struct cracen_spake2p_operation {
	psa_algorithm_t alg;
	cracen_hash_operation_t hash_op; /* Protocol transcript (TT). */
	uint8_t w0[CRACEN_P256_KEY_SIZE];
	uint8_t w1_or_L[CRACEN_P256_POINT_SIZE]; /* w1 is scalar, L is a point. */
	uint8_t xy[CRACEN_P256_KEY_SIZE];
	/* Includes prefix for uncompressed points (0x04). */
	uint8_t XY[CRACEN_P256_POINT_SIZE + 1];
	/* Includes prefix for uncompressed points (0x04). */
	uint8_t YX[CRACEN_P256_POINT_SIZE + 1];
	uint8_t shared[CRACEN_SPAKE2P_HASH_LEN];
	uint8_t shared_len;
	uint8_t KconfPV[CRACEN_SPAKE2P_HASH_LEN];
	uint8_t KconfVP[CRACEN_SPAKE2P_HASH_LEN];
	uint8_t prover[32];
	uint8_t verifier[32];
	uint8_t prover_len;
	uint8_t verifier_len;
	const uint8_t *MN;
	const uint8_t *NM;
	psa_pake_role_t role;
	const struct sx_pk_ecurve *curve;
};
typedef struct cracen_spake2p_operation cracen_spake2p_operation_t;

/** WPA3 SAE operation context. */
struct cracen_wpa3_sae_operation {
	cracen_mac_operation_t mac_op;
	psa_algorithm_t hash_alg;
	uint8_t password[CRACEN_WPA3_SAE_MAX_PWD_LEN];
	uint16_t pw_length;
	uint8_t pwe[CRACEN_P256_POINT_SIZE];
	union {
		struct {
			uint8_t max_id[CRACEN_WPA3_SAE_STA_ID_LEN];
			uint8_t min_id[CRACEN_WPA3_SAE_STA_ID_LEN];
		};
		uint8_t max_id_min_id[2 * CRACEN_WPA3_SAE_STA_ID_LEN];
	};
	uint8_t rand[CRACEN_P256_KEY_SIZE];
	uint8_t kck[CRACEN_P256_KEY_SIZE];
	uint8_t pmk[CRACEN_WPA3_SAE_PMK_LEN];
	uint8_t pmkid[CRACEN_WPA3_SAE_PMKID_SIZE];
	uint8_t commit[CRACEN_WPA3_SAE_COMMIT_SIZE];
	uint8_t peer_commit[CRACEN_WPA3_SAE_COMMIT_SIZE];
	uint8_t hash_length;
	uint8_t pmk_length;
	uint8_t use_h2e:1;
	uint8_t keys_set:1;
	uint8_t salt_set:1;
	uint16_t send_confirm;
	const struct sx_pk_ecurve *curve;
};
typedef struct cracen_wpa3_sae_operation cracen_wpa3_sae_operation_t;

/** PAKE operation context. */
struct cracen_pake_operation {
	psa_algorithm_t alg;
	union {
#ifdef PSA_NEED_CRACEN_SRP_6
		cracen_srp_operation_t cracen_srp_ctx;
#endif /* PSA_NEED_CRACEN_SRP_6 */
#ifdef PSA_NEED_CRACEN_ECJPAKE
		cracen_jpake_operation_t cracen_jpake_ctx;
#endif /* PSA_NEED_CRACEN_ECJPAKE */
#ifdef PSA_NEED_CRACEN_SPAKE2P
		cracen_spake2p_operation_t cracen_spake2p_ctx;
#endif /* PSA_NEED_CRACEN_SPAKE2P */
#ifdef PSA_NEED_CRACEN_WPA3_SAE
		cracen_wpa3_sae_operation_t cracen_wpa3_sae_ctx;
#endif /* PSA_NEED_CRACEN_WPA3_SAE */
		uint8_t _unused;
	};
};
typedef struct cracen_pake_operation cracen_pake_operation_t;

/** Signature representation for ECC and RSA operations.
 *
 * For ECC: r and s point to the two signature components.
 * For RSA: r points to the full signature; s is unused (typically NULL).
 * sz contains the total signature size in bytes.
 */
struct cracen_signature {
	size_t sz;  /** Total signature size, in bytes. */
	uint8_t *r; /** Signature element "r". */
	uint8_t *s; /** Signature element "s". */
};

/** Signature representation for ECC and RSA operations.
 *
 * For ECC: r and s point to the two signature components.
 * For RSA: r points to the full signature; s is unused (typically NULL).
 * sz contains the total signature size in bytes.
 */
struct cracen_const_signature {
	size_t sz;	  /** Total signature size, in bytes. */
	const uint8_t *r; /** Signature element "r". */
	const uint8_t *s; /** Signature element "s". */
};

/** ECC private key representation. */
struct cracen_ecc_priv_key {
	const struct sx_pk_ecurve *curve;
	const uint8_t *d; /** Private key value d. */
};

/** ECC public key representation. */
struct cracen_ecc_pub_key {
	const struct sx_pk_ecurve *curve;
	uint8_t *qx; /** x coordinate of a point on the curve. */
	uint8_t *qy; /** y coordinate of a point on the curve. */
};

/** ECC key pair representation. */
struct cracen_ecc_keypair {
	struct cracen_ecc_priv_key priv_key;
	struct cracen_ecc_pub_key pub_key;
};

/** RSA key representation. */
struct cracen_rsa_key {
	const struct sx_pk_cmd_def *cmd;
	unsigned int slotmask;
	unsigned int dataidx;
	const struct sx_buf *elements[5];
};

/** Plaintext or ciphertext.
 *
 * This structure is used to represent plaintexts and ciphertexts. It is currently used with the
 * functions that implement the RSAES-OAEP and RSAES-PKCS1-v1_5 encryption schemes.
 */
struct cracen_crypt_text {
	uint8_t *addr;
	size_t sz;
};

/** Coprime check parameters. */
struct cracen_coprimecheck {
	const uint8_t *a;
	size_t asz;
	const uint8_t *b;
	size_t bsz;
};

/** RSA prime check parameters. */
struct cracen_rsacheckpq {
	const uint8_t *pubexp;
	size_t pubexpsz;
	uint8_t *p;
	uint8_t *q;
	size_t candidatesz;
	size_t mrrounds;
};

/** RSA prime generation parameters. */
struct cracen_rsagenpq {
	const uint8_t *pubexp;
	size_t pubexpsz;
	uint8_t *p;
	uint8_t *q;
	uint8_t *rndout;
	uint8_t *qptr;
	size_t candidatesz;
	size_t attempts;
};

/** @} */

#endif /* CRACEN_PSA_PRIMITIVES_H */
