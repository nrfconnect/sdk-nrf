/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

#define SX_BLKCIPHER_IV_SZ	(16U)
#define SX_BLKCIPHER_AES_BLK_SZ (16U)

#if defined(PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20)
/* ChaCha20 has a 512 bit block size */
#define SX_BLKCIPHER_MAX_BLK_SZ (64U)
#else
#define SX_BLKCIPHER_MAX_BLK_SZ SX_BLKCIPHER_AES_BLK_SZ
#endif

#define CRACEN_MAX_AES_KEY_SIZE (32u)

#define CRACEN_MAX_AEAD_BLOCK_SIZE (64u)
/*
 * Cracen AEAD uses two backends, BA411 for GCM/CCM and BA417 for
 * ChaChaPoly. And they both have 32 bytes as the max key size.
 */
#define CRACEN_MAX_AEAD_KEY_SIZE   (32u)

/*
 * The low level driver only supports a key size of exactly 32 bytes for
 * CHACHA20 (and for CHACHAPOLY for that sake).
 */
#define CRACEN_MAX_CHACHA20_KEY_SIZE (32u)

/*
 * There is a HW limitation for nRF54LM20A and nRF54LV10A:
 * a maximum of 1 MB of plaintext or ciphertext is supported.
 */
#if defined(CONFIG_SOC_NRF54LM20A) || defined(CONFIG_SOC_NRF54LV10A)
#define CRACEN_MAX_CCM_DATA_SIZE (65536U * SX_BLKCIPHER_AES_BLK_SZ)
#endif /* CONFIG_SOC_NRF54LM20A || CONFIG_SOC_NRF54LV10A */

/*
 * There are two key types supported for ciphers, CHACHA20 and AES,
 * and they both have a max key size of 32.
 */
#define CRACEN_MAX_CIPHER_KEY_SIZE (32u)

/* Max length of PBKDF2 salt. There is no set max length.
 * For this implementation max length is set 128 bytes.
 */
#define CRACEN_PBKDF_MAX_SALT_SIZE 128

/* Max length for HKDF info input. The input is optional and has no set max
 * length. For this implementation max length is set 128 bytes.
 */
#define CRACEN_HKDF_MAX_INFO_SIZE 128

#define CRACEN_CMAC_MAX_LABEL_SIZE 127

#define CRACEN_CMAC_MAX_CONTEXT_SIZE 64

#define CRACEN_JPAKE_USER_ID_MAX_SIZE 16

#define CRACEN_P256_KEY_SIZE   32
#define CRACEN_P256_POINT_SIZE 64

#define CRACEN_TLS12_PRF_MAX_LABEL_SIZE 128
#define CRACEN_TLS12_PRF_MAX_SEED_SIZE	128

#define CRACEN_PRNG_KEY_SIZE		     (32u)  /* Key size to get 256 bits of security */
#define CRACEN_PRNG_ENTROPY_SIZE	     (48u)  /* Seed equals key-len + 16 */
#define CRACEN_PRNG_NONCE_SIZE		     (0u)   /* Not supported for CTR_DRBG */
#define CRACEN_PRNG_PERSONALIZATION_MAX_SIZE (48u)  /* Max Equal to SEED size */
#define CRACEN_PRNG_WORKMEM_SIZE	     (176u) /* KeyLen + AES block len + XORBUF_SZ (128) */
#define CRACEN_PRNG_MAX_REQUEST_SIZE	     (1u << 16) /* Max request. See NIST specification */

#define CRACEN_SPAKE2P_HASH_LEN PSA_HASH_LENGTH(PSA_ALG_SHA_256)

enum cipher_operation {
	CRACEN_DECRYPT,
	CRACEN_ENCRYPT
};

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

	/* CMAC CTR states: */
	CRACEN_KD_STATE_CMAC_CTR_INIT = 0x40,
	CRACEN_KD_STATE_CMAC_CTR_KEY_LOADED,
	CRACEN_KD_STATE_CMAC_CTR_INPUT_LABEL,
	CRACEN_KD_STATE_CMAC_CTR_INPUT_CONTEXT,
	CRACEN_KD_STATE_CMAC_CTR_OUTPUT,

	/* TLS12 PRF states: */
	CRACEN_KD_STATE_TLS12_PRF_INIT = 0x80,
	CRACEN_KD_STATE_TLS12_PRF_OUTPUT,

	/* TLS12 PSK TO MS states: */
	CRACEN_KD_STATE_TLS12_PSK_TO_MS_INIT = 0x100,
	CRACEN_KD_STATE_TLS12_PSK_TO_MS_OUTPUT,

	/* TLS12 EC J-PAKE to PMS state: */
	CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_INIT = 0x200,
	CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_OUTPUT
};

/* States to keep track of when the AEAD sxsymcrypt context has been initialized with xxx_create and
 * when the hardware is reserved with a xxx_resume call.
 */
enum cracen_context_state {
	CRACEN_NOT_INITIALIZED = 0x0,
	CRACEN_CONTEXT_INITIALIZED = 0x1,
	CRACEN_HW_RESERVED = 0x2
};

struct cracen_hash_operation_s {
	const struct sxhashalg *sx_hash_algo;

	/* Internal sxcrypto context */
	struct sxhash sx_ctx;

	/* The driver can perform a processing round after getting a multiple of
	 * the blocksize.
	 * Therefore we have to know how much much data is left to fill up to
	 * the next block
	 */
	size_t bytes_left_for_next_block;

	/* Buffer for input data to fill up the next block */
	uint8_t input_buffer[SX_HASH_MAX_ENABLED_BLOCK_SIZE];

	/* Flag to know if the Hashing has already started */
	bool is_first_block;
};
typedef struct cracen_hash_operation_s cracen_hash_operation_t;

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
};
typedef struct cracen_aead_operation cracen_aead_operation_t;

struct cracen_cmac_context_s {
	uint8_t mac_state[SX_BLKCIPHER_AES_BLK_SZ];	   /*  The current MAC state */
	uint8_t partial_block[SX_BLKCIPHER_AES_BLK_SZ];
	size_t partial_len;
	size_t processed_len;	   /*  Total length of data processed so far (in bytes) */
};
typedef struct cracen_cmac_context_s cracen_cmac_context_t;

struct cracen_mac_operation_s {
	psa_algorithm_t alg;
	size_t mac_size;

	/* The driver can perform a processing round after getting a multiple of
	 * the blocksize.
	 * Therefore we have to know how much much data is left to fill up to
	 * the next block
	 */
	size_t bytes_left_for_next_block;

	/* Buffer for input data to fill up the next block */
	uint8_t input_buffer[SX_MAX(SX_HASH_MAX_ENABLED_BLOCK_SIZE, SX_BLKCIPHER_PRIV_SZ)];

	bool is_first_block;
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
#if defined(CONFIG_CRACEN_NEED_MULTIPART_WORKAROUNDS)
			cracen_cmac_context_t sw_ctx;
			struct sxblkcipher cipher;
#endif /* CONFIG_CRACEN_NEED_MULTIPART_WORKAROUNDS */
		} cmac;
#endif /* PSA_NEED_CRACEN_CMAC */
		uint8_t _unused;
	};
};
typedef struct cracen_mac_operation_s cracen_mac_operation_t;

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
		struct {
			uint8_t blk_counter;
			uint8_t prk[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
			uint8_t t[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
			char info[CRACEN_HKDF_MAX_INFO_SIZE];
			size_t info_length;
			bool info_set;
		} hkdf;

		struct {
			uint64_t input_cost;
			char password[SX_HASH_MAX_ENABLED_BLOCK_SIZE];
			size_t password_length;
			char salt[CRACEN_PBKDF_MAX_SALT_SIZE];
			size_t salt_length;
			uint32_t blk_counter;
			uint8_t uj[PSA_MAC_MAX_SIZE];
			uint8_t tj[PSA_MAC_MAX_SIZE];
		} pbkdf2;

		struct {
			uint8_t key_buffer[CRACEN_MAX_AES_KEY_SIZE];
			struct sxkeyref keyref;
			struct sx_pk_cnx *pk_cnx;
			uint32_t counter;
			/* The +1 here is meant to store an algorithm specific byte needed after the
			 * label
			 */
			uint8_t label[CRACEN_CMAC_MAX_LABEL_SIZE + 1];
			size_t label_length;
			uint8_t context[CRACEN_CMAC_MAX_CONTEXT_SIZE];
			size_t context_length;
			uint32_t L;
			uint8_t K_0[SX_BLKCIPHER_AES_BLK_SZ];
		} cmac_ctr;

		struct {
			uint8_t key[32];
		} ecjpake_to_pms;

		struct {
			/* May contain secret, length of secret as uint16be, other secret
			 * and other secret length as uint16be.
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
	};
};
typedef struct cracen_key_derivation_operation cracen_key_derivation_operation_t;

struct cracen_jpake_operation {
	psa_algorithm_t alg;		      /* Hashing algorithm. */
	uint8_t secret[CRACEN_P256_KEY_SIZE]; /* A secret value derived from the password. */
	uint8_t user_id[CRACEN_JPAKE_USER_ID_MAX_SIZE];
	uint8_t peer_id[CRACEN_JPAKE_USER_ID_MAX_SIZE];
	size_t user_id_length;
	size_t peer_id_length;
	uint8_t rd_idx;
	uint8_t wr_idx;
	uint8_t x[3][CRACEN_P256_KEY_SIZE];   /* private keys */
	uint8_t X[3][CRACEN_P256_POINT_SIZE]; /* public keys */
	uint8_t P[3][CRACEN_P256_POINT_SIZE]; /* peer keys */
	uint8_t V[CRACEN_P256_POINT_SIZE];
	uint8_t r[CRACEN_P256_KEY_SIZE];
	const struct sx_pk_ecurve *curve;
};
typedef struct cracen_jpake_operation cracen_jpake_operation_t;

/** @brief Structure type for PRNG context */
struct cracen_prng_context {
	uint8_t key[CRACEN_PRNG_KEY_SIZE];
	uint8_t V[SX_BLKCIPHER_AES_BLK_SZ];
	uint64_t reseed_counter;
	uint32_t initialized;
};
typedef struct cracen_prng_context cracen_prng_context_t;

#define CRACEN_SRP_HASH_ALG		 PSA_ALG_SHA_512
#define CRACEN_SRP_HASH_LENGTH		 PSA_HASH_LENGTH(CRACEN_SRP_HASH_ALG)
#define CRACEN_SRP_RFC3526_KEY_BITS_SIZE 3072
#define CRACEN_SRP_FIELD_SIZE		 PSA_BITS_TO_BYTES(CRACEN_SRP_RFC3526_KEY_BITS_SIZE)
/* The same number that Oberon PSA driver use to align the implementations */
#define CRACEN_SRP_MAX_SALT_LENGTH	 64

struct cracen_srp_operation {
	psa_pake_role_t role;			   /* Protocol role */
	uint8_t user_hash[CRACEN_SRP_HASH_LENGTH]; /* Hash of the username */
	uint8_t salt[CRACEN_SRP_MAX_SALT_LENGTH];  /* salt chosen by server */
	size_t salt_len;			   /* length of salt */
	union {
		uint8_t x[CRACEN_SRP_HASH_LENGTH]; /* x = SHA512(salt | SHA512(user | ":" | pass))
						    */
		uint8_t v[CRACEN_SRP_FIELD_SIZE];  /* v = g^x mod N */
	};
	uint8_t ab[32];			  /* random a for client, b for server*/
	uint8_t A[CRACEN_SRP_FIELD_SIZE]; /* (v + g^b) mod N */
	uint8_t B[CRACEN_SRP_FIELD_SIZE]; /* g^a mod N */

	uint8_t M[CRACEN_SRP_HASH_LENGTH]; /* H(H(N) xor H(g) | H(U) | salt | A | B | K ) */
	uint8_t K[CRACEN_SRP_HASH_LENGTH]; /* shared key k= H_Interleaved(S) */
};
typedef struct cracen_srp_operation cracen_srp_operation_t;

struct cracen_spake2p_operation {
	psa_algorithm_t alg;
	cracen_hash_operation_t hash_op; /* Protocol transcript (TT) */
	uint8_t w0[CRACEN_P256_KEY_SIZE];
	uint8_t w1_or_L[CRACEN_P256_POINT_SIZE]; /* w1 is scalar, L is a point. */
	uint8_t xy[CRACEN_P256_KEY_SIZE];
	uint8_t XY[CRACEN_P256_POINT_SIZE + 1]; /* Includes prefix for uncompressed points (0x04) */
	uint8_t YX[CRACEN_P256_POINT_SIZE + 1]; /* Includes prefix for uncompressed points (0x04) */
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

struct cracen_pake_operation {
	psa_algorithm_t alg;
	union {
#ifdef CONFIG_PSA_NEED_CRACEN_SRP_6
		cracen_srp_operation_t cracen_srp_ctx;
#endif /* CONFIG_PSA_NEED_CRACEN_SRP_6 */
#ifdef CONFIG_PSA_NEED_CRACEN_ECJPAKE
		cracen_jpake_operation_t cracen_jpake_ctx;
#endif /* CONFIG_PSA_NEED_CRACEN_ECJPAKE */
#ifdef CONFIG_PSA_NEED_CRACEN_SPAKE2P
		cracen_spake2p_operation_t cracen_spake2p_ctx;
#endif /* CONFIG_PSA_NEED_CRACEN_SPAKE2P */
		uint8_t _unused;
	};
};
typedef struct cracen_pake_operation cracen_pake_operation_t;

struct cracen_signature {
	size_t sz;  /** Total signature size, in bytes. */
	uint8_t *r; /** Signature element "r". */
	uint8_t *s; /** Signature element "s". */
};

struct cracen_ecc_priv_key {
	const struct sx_pk_ecurve *curve;
	const uint8_t *d; /** Private key value d */
};

struct cracen_ecc_pub_key {
	const struct sx_pk_ecurve *curve;
	uint8_t *qx; /** x coordinate of a point on the curve */
	uint8_t *qy; /** y coordinate of a point on the curve */
};

struct cracen_ecc_keypair {
	struct cracen_ecc_priv_key priv_key;
	struct cracen_ecc_pub_key pub_key;
};

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

struct cracen_coprimecheck {
	const uint8_t *a;
	size_t asz;
	const uint8_t *b;
	size_t bsz;
};

struct cracen_rsacheckpq {
	const uint8_t *pubexp;
	size_t pubexpsz;
	uint8_t *p;
	uint8_t *q;
	size_t candidatesz;
	size_t mrrounds;
};

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

#endif /* CRACEN_PSA_PRIMITIVES_H */
