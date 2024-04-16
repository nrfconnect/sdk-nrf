/* Signature scheme RSASSA-PKCS1-v1_5, based on RFC 8017.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSASSA-PKCS1-v1_5 sign and verify tasks:
 *      1. message digest (size: digest size of selected hash algorithm).
 *
 * Notes
 * - In the signature verify task, the integer representative of the user-
 *   provided signature should be <= than the RSA modulus. If this is not the
 *   case, the signature is not valid: this error is detected by the BA414ep
 *   core and SilexPK will return the status code SX_ERR_OUT_OF_RANGE.
 */

#include <string.h>
#include <silexpk/iomem.h>
#include <silexpk/core.h>
#include <sxsymcrypt/sha1.h>
#include <sxsymcrypt/sha2.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/rsassa_pkcs1v15.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include "../include/sicrypto/mem.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"
#include "rsakey.h"
#include "sigdefs.h"

/* Return a pointer to the DER encoding of the hash algorithm ID and write its
 * size to 'dersz'. In case of error, the value written through 'dersz' is 0.
 * RFC 8017 specifies 7 recommended hash functions: SHA1, SHA2-224, SHA2-256,
 * SHA2-384, SHA2-512, SHA2-512/224, SHA2-512/256.
 * Note: SHA2-512/224, SHA2-512/256 are not in sxsymcrypt
 */
static const unsigned char *get_hash_der(const struct sxhashalg *hashalg, size_t *dersz)
{
	static const unsigned char der_sha1[] = {0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
						 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14};
	static const unsigned char der_sha2_224[] = {0x30, 0x2D, 0x30, 0x0d, 0x06, 0x09, 0x60,
						     0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						     0x04, 0x05, 0x00, 0x04, 0x1C};
	static const unsigned char der_sha2_256[] = {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60,
						     0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						     0x01, 0x05, 0x00, 0x04, 0x20};
	static const unsigned char der_sha2_384[] = {0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60,
						     0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						     0x02, 0x05, 0x00, 0x04, 0x30};
	static const unsigned char der_sha2_512[] = {0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60,
						     0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						     0x03, 0x05, 0x00, 0x04, 0x40};
	const unsigned char *der;
	size_t der_size = sizeof(der_sha2_224);

	if (hashalg == &sxhashalg_sha1) {
		der = der_sha1;
		der_size = sizeof(der_sha1);
	} else if (hashalg == &sxhashalg_sha2_224) {
		der = der_sha2_224;
	} else if (hashalg == &sxhashalg_sha2_256) {
		der = der_sha2_256;
	} else if (hashalg == &sxhashalg_sha2_384) {
		der = der_sha2_384;
	} else if (hashalg == &sxhashalg_sha2_512) {
		der = der_sha2_512;
	} else {
		/* error: unsupported hash algorithm */
		der_size = 0;
		der = NULL;
	}

	*dersz = der_size;
	return der;
}

static int rsassa_pkcs1v15_verify_continue(struct sitask *t, struct siwq *wq)
{
	int r;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsassa_pkcs1v15.key);
	const char **outputs = sx_pk_get_output_ops(t->pk);
	size_t paddingstrsz, dersz;
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* get reference DER encoding of the hash algorithm ID */
	const unsigned char *refder = get_hash_der(t->hashalg, &dersz);

	if (dersz == 0) {
		sx_pk_release_req(t->pk);
		return (t->statuscode = SX_ERR_UNSUPPORTED_HASH_ALG);
	}

	/* set pointer to encoded message (EM in RFC 8017), assumed big endian
	 */
	const char *const encodedmsgstart = outputs[0];

	/* set pointer to the first octet beyond the encoded message */
	const char *const encodedmsgend = encodedmsgstart + modulussz;

	/* pointer used to walk through the EM */
	char *encmsg = (char *)encodedmsgstart;

	/* invalid signature if first two bytes of EM are not 0x00 0x01 */
	r = (sx_rdpkmem_byte(encmsg) != 0x00);
	encmsg++;

	r |= (sx_rdpkmem_byte(encmsg) != 0x1);
	encmsg++;

	/* check the presence of the padding string (PS in RFC 8017) */
	while ((sx_rdpkmem_byte(encmsg) == 0xFF) && (encmsg < encodedmsgend)) {
		encmsg++;
	}

	/* compute size of the padding string PS */
	paddingstrsz = ((size_t)(encmsg - encodedmsgstart)) - 2;

	/* The size of the PS must match the empty space in the EM. This ensures
	 * that no other information follows the hash value in the EM. This check is
	 * required by FIPS 186-4, section 5.5, point f.
	 */
	if (paddingstrsz != (modulussz - 3 - dersz - digestsz)) {
		sx_pk_release_req(t->pk);
		return (t->statuscode = SX_ERR_INVALID_SIGNATURE);
	}

	/* there must be a zero byte separating PS from T */
	r |= (sx_rdpkmem_byte(encmsg) != 0x00);
	encmsg++;

	/* check the DER encoding of the hash algorithm ID (first part of T) */
	r |= si_memdiff(encmsg, (const char *)refder, dersz);
	encmsg += dersz;

	/* check the hash part of T: reference hash value is in workmem */
	r |= si_memdiff(encmsg, t->workmem, digestsz);

	sx_pk_release_req(t->pk);

	if (r == 0) {
		return (t->statuscode = SX_OK);
	} else {
		return (t->statuscode = SX_ERR_INVALID_SIGNATURE);
	}
}

static void rsassa_pkcs1v15_verify_modexp(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	struct siparams_rsassa_pkcs1v15 *par = &t->params.rsassa_pkcs1v15;
	int sizes[6] = {0};

	si_wq_run_after(t, &par->wq, rsassa_pkcs1v15_verify_continue);

	pkreq = sx_pk_acquire_req(par->key->cmd);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}
	si_ffkey_write_sz(par->key, sizes);
	SI_FFKEY_REFER_INPUT(par->key, sizes) = par->sig->sz;
	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, inputs);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	si_ffkey_write(par->key, inputs);
	sx_wrpkmem(SI_FFKEY_REFER_INPUT(par->key, inputs).addr, par->sig->r, par->sig->sz);

	/* start the modular exponentiation s^e mod n (RSAVP1 verify primitive)
	 */
	sx_pk_run(pkreq.req);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
}

static void rsassa_pkcs1v15_hash_message(struct sitask *t)
{
	/* Override the statuscode value to be able to run si_task_produce(). */
	t->statuscode = SX_ERR_READY;

	/* Start hash computation, digest result in workmem. */
	si_task_produce(t, t->workmem, sx_hash_get_alg_digestsz(t->hashalg));
}

static int rsassa_pkcs1v15_verify_init(struct sitask *t, const struct si_sig_pubkey *pubkey,
				       const struct si_sig_signature *signature)
{
	const struct si_rsa_key *rsakey = &pubkey->key.rsa;

	if (t->workmemsz < sx_hash_get_alg_digestsz(pubkey->hashalg)) {
		return si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
	}

	/* the signature must not be longer than the modulus (modified step 1 of
	 * RSASSA-PKCS1-V1_5-VERIFY)
	 */
	if (signature->sz > SI_RSA_KEY_OPSZ(rsakey)) {
		return si_task_mark_final(t, SX_ERR_INVALID_SIGNATURE);
	}

	t->hashalg = pubkey->hashalg;
	t->params.rsassa_pkcs1v15.key = rsakey;
	t->params.rsassa_pkcs1v15.sig = (struct si_sig_signature *)signature;

	return SX_OK;
}

static void rsassa_pkcs1v15_verify_consume_digest(struct sitask *t, const char *data, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);

	if (sz < digestsz) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	if (sz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* Copy the message digest to workmem. */
	memcpy(t->workmem, data, sz);

	t->actions.consume = NULL;
	t->actions.run = rsassa_pkcs1v15_verify_modexp;
}

static void si_sig_create_rsa_pkcs1v15_verify_digest(struct sitask *t,
						     const struct si_sig_pubkey *pubkey,
						     const struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = rsassa_pkcs1v15_verify_consume_digest;

	rsassa_pkcs1v15_verify_init(t, pubkey, signature);
}

static int rsassa_pkcs1v15_verify_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	rsassa_pkcs1v15_verify_modexp(t);

	return t->statuscode;
}

static void si_sig_create_rsa_pkcs1v15_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
					      const struct si_sig_signature *signature)
{
	int r;

	r = rsassa_pkcs1v15_verify_init(t, pubkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.rsassa_pkcs1v15.wq, rsassa_pkcs1v15_verify_start);

	si_hash_create(t, pubkey->hashalg);
	t->actions.run = rsassa_pkcs1v15_hash_message;
}

static int rsassa_pkcs1v15_sign_finish(struct sitask *t, struct siwq *wq)
{
	char **outputs = (char **)sx_pk_get_output_ops(t->pk);
	struct siparams_rsassa_pkcs1v15 *par = &t->params.rsassa_pkcs1v15;
	(void)wq;

	/* outputs[0] points to the signature, which we assume big endian */
	sx_rdpkmem(par->sig->r, outputs[0], SI_RSA_KEY_OPSZ(par->key));

	par->sig->sz = SI_RSA_KEY_OPSZ(par->key);

	sx_pk_release_req(t->pk);

	return t->statuscode;
}

/* Complete message encoding and start the modular exponentiation. */
static void rsassa_pkcs1v15_sign_continue(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	struct siparams_rsassa_pkcs1v15 *par = &t->params.rsassa_pkcs1v15;
	size_t modulussz = SI_RSA_KEY_OPSZ(par->key);
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	int sizes[6] = {0};
	size_t paddingstrsz, dersz, i;
	const unsigned char *der;
	unsigned char *encmsg;

	/* get pointer to the DER encoding of the hash algorithm ID */
	der = get_hash_der(t->hashalg, &dersz);
	if (dersz == 0) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* step 3 of EMSA-PKCS1-v1_5-ENCODE */
	if (modulussz < dersz + digestsz + 11) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	pkreq = sx_pk_acquire_req(par->key->cmd);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	si_ffkey_write_sz(par->key, sizes);
	SI_FFKEY_REFER_INPUT(par->key, sizes) = modulussz;

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, inputs);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	/* copy modulus and exponent to device memory */
	si_ffkey_write(par->key, inputs);

	/* encmsg is used to write the encoded message in device memory */
	encmsg = (unsigned char *)SI_FFKEY_REFER_INPUT(par->key, inputs).addr;

	/* start preparing the encoded message in device memory */
	sx_wrpkmem_byte(encmsg, 0x0);
	encmsg++;
	sx_wrpkmem_byte(encmsg, 0x1);
	encmsg++;

	/* write the padding string */
	paddingstrsz = modulussz - dersz - digestsz - 3;
	for (i = 0; i < paddingstrsz; i++) {
		sx_wrpkmem_byte(encmsg, 0xFF);
		encmsg++;
	}

	sx_wrpkmem_byte(encmsg, 0x0);
	encmsg++;

	/* write the DER encoding of the hash algorithm ID */
	sx_wrpkmem(encmsg, der, dersz);
	encmsg += dersz;

	/* write the message digest */
	sx_wrpkmem(encmsg, t->workmem, digestsz);

	si_wq_run_after(t, &par->wq, rsassa_pkcs1v15_sign_finish);

	/* start the modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_pk_run(pkreq.req);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
}

static void rsassa_pkcs1v15_sign_consume_digest(struct sitask *t, const char *data, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);

	if (sz < digestsz) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	if (sz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* Copy the message digest to workmem. */
	memcpy(t->workmem, data, sz);

	t->actions.consume = NULL;
	t->actions.run = rsassa_pkcs1v15_sign_continue;
}

static int rsassa_pkcs1v15_sign_init(struct sitask *t, const struct si_sig_privkey *privkey,
				     struct si_sig_signature *signature)
{
	if (t->workmemsz < sx_hash_get_alg_digestsz(privkey->hashalg)) {
		return si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
	}

	t->hashalg = privkey->hashalg;
	t->params.rsassa_pkcs1v15.key = &privkey->key.rsa;
	t->params.rsassa_pkcs1v15.sig = signature;

	return SX_OK;
}

static void si_sig_create_rsa_pkcs1v15_sign_digest(struct sitask *t,
						   const struct si_sig_privkey *privkey,
						   struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = rsassa_pkcs1v15_sign_consume_digest;

	rsassa_pkcs1v15_sign_init(t, privkey, signature);
}

static int rsassa_pkcs1v15_sign_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	rsassa_pkcs1v15_sign_continue(t);

	return t->statuscode;
}

static void si_sig_create_rsa_pkcs1v15_sign(struct sitask *t, const struct si_sig_privkey *privkey,
					    struct si_sig_signature *signature)
{
	int r;

	r = rsassa_pkcs1v15_sign_init(t, privkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.rsassa_pkcs1v15.wq, rsassa_pkcs1v15_sign_start);

	si_hash_create(t, privkey->hashalg);
	t->actions.run = rsassa_pkcs1v15_hash_message;
}

static unsigned short int get_key_opsz(const struct si_sig_privkey *privkey)
{
	const struct si_rsa_key *key = &(privkey->key.rsa);

	return SI_RSA_KEY_OPSZ(key);
}

const struct si_sig_def *const si_sig_def_rsa_pkcs1v15 = &(const struct si_sig_def){
	.sign = si_sig_create_rsa_pkcs1v15_sign,
	.sign_digest = si_sig_create_rsa_pkcs1v15_sign_digest,
	.verify = si_sig_create_rsa_pkcs1v15_verify,
	.verify_digest = si_sig_create_rsa_pkcs1v15_verify_digest,
	.getkeyopsz = get_key_opsz,
	.sigcomponents = 1,
};
