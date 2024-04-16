/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_INTERNAL_HEADER_FILE
#define SICRYPTO_INTERNAL_HEADER_FILE

#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/aead.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/internal.h>
#include <silexpk/ec_curves.h>
#include <stdbool.h>
#include <stdint.h>

struct sienv;
struct sitask;

struct siwq {
	struct siwq *next;
	int (*run)(struct sitask *src, struct siwq *w);
};

struct siparams_drbghash {
	int progress;
	size_t seedlen;
	const char *pers;
	size_t perssz;
	const char *entropy;
	size_t entropysz;
	const char *nonce;
	size_t noncesz;
	uint64_t reseed_ctr;
	int hashdf_iterations;
	size_t outsz;
	struct siwq wq;
};

struct siparams_drbgctr {
	size_t processed;
	size_t keysz;
	size_t outsz;
	uint64_t reseed_interval;
	struct siwq wq;
};

struct siparams_prng_predefined {
	char *buf;
	size_t bufsz;
	size_t readoffset;
};

struct siparams_hmac {
	struct siwq wq;
};

struct siparams_pbkdf2 {
	struct siparams_hmac hmac;
	size_t iterations;
	size_t iter_ctr;
	const char *salt;
	size_t saltsz;
	char *out;
	size_t outsz;
	const char *pswd;
	size_t pswdsz;
	struct siwq wq;
};

struct siparams_rndinrange {
	size_t rndsz;
	const unsigned char *n;
	unsigned char msb_mask;
	struct siwq wq;
};

struct siparams_ecdsa_sign {
	union {
		struct siparams_rndinrange rndnumber;
		struct siparams_hmac hmac;
	};
	struct si_sig_signature *signature;
	const struct si_sig_privkey *privkey;
	struct siwq wq;
	int attempts;
	uint16_t tlen;
	uint8_t deterministic_hmac_step;
	int deterministic_retries;
	bool deterministic;
};

struct siparams_ecdsa_ver {
	const struct si_sig_pubkey *pubkey;
	const struct si_sig_signature *signature;
	struct siwq wq;
};

struct siparams_ik {
	const struct si_sig_privkey *privkey;
	struct si_sig_signature *signature;
	struct si_sig_pubkey *pubkey;
	int statuscode;
	struct siwq wq;
	int attempts;
};

struct siparams_mgf1xor {
	const char *seed;
	size_t seedsz;
	char *xorinout;
	size_t masksz;
	struct siwq wq;
};

struct siparams_rsapss {
	struct siparams_mgf1xor mgf1xor;
	const struct si_rsa_key *key;
	struct si_sig_signature *sig;
	size_t saltsz;
	size_t emsz;
	struct siwq wq;
};

struct siparams_rsassa_pkcs1v15 {
	const struct si_rsa_key *key;
	struct si_sig_signature *sig;
	struct siwq wq;
};

struct siparams_rsaes_pkcs1v15 {
	struct si_rsa_key *key;
	struct si_ase_text *text;
	unsigned long retryctr;
	struct siwq wq;
};

struct siparams_ed25519_verif {
	struct sx_ed25519_pt *pubkey;
	const struct si_sig_signature *signature;
	struct siwq wq;
};

struct siparams_ed25519_sign {
	const struct sx_ed25519_v *privkey;
	struct si_sig_signature *signature;
	const char *msg;
	size_t msgsz;
	struct siwq wq;
};

struct siparams_ed25519_pubkey {
	const struct sx_ed25519_v *privkey;
	struct sx_ed25519_pt *pubkey;
	struct siwq wq;
};

struct siparams_x25519_pubkey {
	const struct sx_x25519_op *privkey;
	struct sx_x25519_pt *pubkey;
	struct siwq wq;
};

struct siparams_ed448_verif {
	const struct sx_ed448_pt *pubkey;
	const struct si_sig_signature *signature;
	struct siwq wq;
};

struct siparams_ed448_sign {
	const struct sx_ed448_v *privkey;
	struct si_sig_signature *signature;
	const char *msg;
	size_t msgsz;
	struct siwq wq;
};

struct siparams_ed448_pubkey {
	const struct sx_ed448_v *privkey;
	struct sx_ed448_pt *pubkey;
	struct siwq wq;
};

struct siparams_x448_pubkey {
	const struct sx_x448_op *privkey;
	struct sx_x448_pt *pubkey;
	struct siwq wq;
};

struct siparams_hkdf {
	struct siparams_hmac hmac;
	const char *info;
	size_t infosz;
	char *output;
	size_t outsz;
	struct siwq wq;
};

struct siparams_kdf2 {
	char *output;
	int outsz;
	const char *ikm;
	int ikmsz;
	const char *info;
	int infosz;
	struct siwq wq;
};

struct siparams_rsaoaep {
	struct siparams_mgf1xor mgf1xor;
	struct si_ase_text *text;
	struct sx_buf *label;
	struct si_rsa_key *key;
	struct siwq wq;
};

struct siparams_ecc {
	struct siparams_rndinrange rndnumber;
	struct si_eccpk *pk;
	struct si_eccsk *sk;
	int attempts;
	struct siwq wq;
};

struct siparams_coprimecheck {
	const char *a;
	size_t asz;
	const char *b;
	size_t bsz;
	struct siwq wq;
};

struct siparams_rsacheckpq {
	union {
		struct siparams_rndinrange rndinrange;
		struct siparams_coprimecheck coprimecheck;
	} subtask_params;
	const char *pubexp;
	size_t pubexpsz;
	char *p;
	char *q;
	size_t candidatesz;
	size_t mrrounds;
	struct siwq wq;
};

struct siparams_rsagenpq {
	struct siparams_rsacheckpq checkpq;
	const char *pubexp;
	size_t pubexpsz;
	char *p;
	char *q;
	char *rndout;
	char *qptr;
	size_t candidatesz;
	size_t attempts;
	struct siwq wq;
};

struct siparams_rsagenprivkey {
	struct siparams_rsagenpq genpq;
	const char *pubexp;
	size_t pubexpsz;
	struct si_rsa_key *key;
	size_t keysz;
	struct siwq wq;
};

struct siactions {
	int (*status)(struct sitask *t);
	int (*wait)(struct sitask *t);
	void (*consume)(struct sitask *t, const char *msg, size_t sz);
	void (*partial_run)(struct sitask *t);
	void (*run)(struct sitask *t);
	void (*produce)(struct sitask *t, char *out, size_t sz);
};

struct sitask {
	int statuscode;
	int partialrun;
	union {
		struct sxhash h;
		struct sxblkcipher b;
		struct sxaead c;
	} u;
	struct sx_pk_req *pk;
	struct siactions actions;
	void *usrctx;
	const struct sxhashalg *hashalg;
	char *workmem;
	size_t workmemsz;
	union {
		struct siparams_drbghash drbghash;
		struct siparams_drbgctr drbgctr;
		struct siparams_prng_predefined prng_pred;
		struct siparams_ecdsa_sign ecdsa_sign;
		struct siparams_ecdsa_ver ecdsa_ver;
		struct siparams_ed25519_verif ed25519_verif;
		struct siparams_ed25519_sign ed25519_sign;
		struct siparams_ed25519_pubkey ed25519_pubkey;
		struct siparams_x25519_pubkey x25519_pubkey;
		struct siparams_ed448_verif ed448_verif;
		struct siparams_ed448_sign ed448_sign;
		struct siparams_ed448_pubkey ed448_pubkey;
		struct siparams_x448_pubkey x448_pubkey;
		struct siparams_ik ik;
		struct siparams_hmac hmac;
		struct siparams_hkdf hkdf;
		struct siparams_pbkdf2 pbkdf2;
		struct siparams_rsapss rsapss;
		struct siparams_rsassa_pkcs1v15 rsassa_pkcs1v15;
		struct siparams_rsaes_pkcs1v15 rsaes_pkcs1v15;
		struct siparams_rndinrange rndinrange;
		struct siparams_mgf1xor mgf1xor;
		struct siparams_rsaoaep rsaoaep;
		struct siparams_kdf2 kdf2;
		struct siparams_ecc ecc;
		struct siparams_coprimecheck coprimecheck;
		struct siparams_rsacheckpq rsacheckpq;
		struct siparams_rsagenpq rsagenpq;
		struct siparams_rsagenprivkey rsagenprivkey;
	} params;
	struct siwq *wq;

	char *output;
};

struct sienv {
	struct sx_pk_cnx *cnx;
};

#endif
