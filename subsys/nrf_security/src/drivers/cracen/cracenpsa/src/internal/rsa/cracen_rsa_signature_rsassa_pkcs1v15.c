/* Signature scheme RSASSA-PKCS1-v1_5, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSASSA-PKCS1-v1_5 sign and verify:
 *      1. message digest (size: digest size of selected hash algorithm).
 *
 * Notes
 * - In the signature verify function, the integer representative of the user-
 *   provided signature should be <= than the RSA modulus. If this is not the
 *   case, the signature is not valid: this error is detected by the BA414ep
 *   core and SilexPK will return the status code SX_ERR_OUT_OF_RANGE.
 */

#include <internal/rsa/cracen_rsa_key.h>
#include <internal/rsa/cracen_rsa_signature_pkcs1v15.h>
#include <internal/rsa/cracen_rsa_common.h>

#include <string.h>
#include <silexpk/iomem.h>
#include <silexpk/core.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

#define WORKMEM_SIZE (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + 2 * PSA_HASH_MAX_SIZE + 4)
#define FIXED_BYTES  3
#define INPUT_SLOTS  6
#define HEADER_BYTES 2

/* Return a pointer to the DER encoding of the hash algorithm ID and write its
 * size to 'dersz'. In case of error, the value written through 'dersz' is 0.
 * RFC 8017 specifies 7 recommended hash functions: SHA1, SHA2-224, SHA2-256,
 * SHA2-384, SHA2-512, SHA2-512/224, SHA2-512/256.
 * Note: SHA2-512/224, SHA2-512/256 are not in sxsymcrypt
 */
static const uint8_t *get_hash_der(const struct sxhashalg *hashalg, size_t *dersz)
{
	static const uint8_t der_sha1[15] = {0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
					     0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14};

	static const uint8_t der_sha2_224[19] = {0x30, 0x2D, 0x30, 0x0d, 0x06, 0x09, 0x60,
						 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						 0x04, 0x05, 0x00, 0x04, 0x1C};

	static const uint8_t der_sha2_256[19] = {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60,
						 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						 0x01, 0x05, 0x00, 0x04, 0x20};

	static const uint8_t der_sha2_384[19] = {0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60,
						 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						 0x02, 0x05, 0x00, 0x04, 0x30};

	static const uint8_t der_sha2_512[19] = {0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60,
						 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
						 0x03, 0x05, 0x00, 0x04, 0x40};

	const uint8_t *der;
	/* der_size is 19 in all cases except sha1, so we set it here to avoid duplicate code*/
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

int cracen_rsa_pkcs1v15_sign_message(struct cracen_rsa_key *rsa_key,
				     struct cracen_signature *signature,
				     const struct sxhashalg *hashalg, const uint8_t *message,
				     size_t message_length)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_rsa_pkcs1v15_sign_digest(rsa_key, signature, hashalg, digest, digestsz);
}

int cracen_rsa_pkcs1v15_sign_digest(struct cracen_rsa_key *rsa_key,
				    struct cracen_signature *signature,
				    const struct sxhashalg *hashalg, const uint8_t *digest,
				    size_t digest_length)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	uint8_t workmem[WORKMEM_SIZE];

	if (sx_hash_get_alg_digestsz(hashalg) > WORKMEM_SIZE) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}
	if (digest_length < digestsz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	memcpy(workmem, digest, digestsz);

	/* Complete message encoding and start the modular exponentiation. */
	sx_pk_req req;
	struct sx_pk_slot inputs[INPUT_SLOTS];
	int input_sizes[INPUT_SLOTS] = {0};
	size_t paddingstrsz;
	size_t dersz;
	const uint8_t *der;
	uint8_t *encmsg;

	/* get pointer to the DER encoding of the hash algorithm ID */
	der = get_hash_der(hashalg, &dersz);
	if (dersz == 0) {
		return SX_ERR_INVALID_ARG;
	}

	/* step 3 of EMSA-PKCS1-v1_5-ENCODE */
	if (modulussz < dersz + digestsz + 11) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, rsa_key->cmd);

	cracen_ffkey_write_sz(rsa_key, input_sizes);
	CRACEN_FFKEY_REFER_INPUT(rsa_key, input_sizes) = modulussz;

	status = sx_pk_list_gfp_inslots(&req, input_sizes, inputs);
	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	cracen_ffkey_write(rsa_key, inputs);
	encmsg = (uint8_t *)CRACEN_FFKEY_REFER_INPUT(rsa_key, inputs).addr;

	/* start preparing the encoded message in device memory */
	sx_wrpkmem_byte(encmsg, 0x0);
	encmsg++;
	sx_wrpkmem_byte(encmsg, 0x1);
	encmsg++;

	/* write the padding string, removing the 3 fixed bytes */
	paddingstrsz = modulussz - dersz - digestsz - FIXED_BYTES;
	for (size_t i = 0; i < paddingstrsz; i++) {
		sx_wrpkmem_byte(encmsg, 0xFF);
		encmsg++;
	}

	sx_wrpkmem_byte(encmsg, 0x0);
	encmsg++;

	/* write the DER encoding of the hash algorithm ID */
	sx_wrpkmem(encmsg, der, dersz);
	encmsg += dersz;

	sx_wrpkmem(encmsg, workmem, digestsz);

	/* start the modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_pk_run(&req);
	status = sx_pk_wait(&req);
	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	uint8_t **outputs = (uint8_t **)sx_pk_get_output_ops(&req);

	/* outputs[0] points to the signature, which we assume big endian */
	sx_rdpkmem(signature->r, outputs[0], CRACEN_RSA_KEY_OPSZ(rsa_key));

	signature->sz = CRACEN_RSA_KEY_OPSZ(rsa_key);

	sx_pk_release_req(&req);

	return SX_OK;
}

int cracen_rsa_pkcs1v15_verify_message(struct cracen_rsa_key *rsa_key,
				       struct cracen_const_signature *signature,
				       const struct sxhashalg *hashalg, const uint8_t *message,
				       size_t message_length)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_rsa_pkcs1v15_verify_digest(rsa_key, signature, hashalg, digest, digestsz);
}

int cracen_rsa_pkcs1v15_verify_digest(struct cracen_rsa_key *rsa_key,
				      struct cracen_const_signature *signature,
				      const struct sxhashalg *hashalg, const uint8_t *digest,
				      size_t digest_length)
{
	int status;
	int r;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	/* Workmem for RSA signature is rsa_modulus_size + 2*hash_digest_size + 4 */
	uint8_t workmem[WORKMEM_SIZE];

	if (WORKMEM_SIZE < sx_hash_get_alg_digestsz(hashalg)) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* the signature must not be longer than the modulus (modified step 1 of
	 * RSASSA-PKCS1-V1_5-VERIFY)
	 */
	if (signature->sz > CRACEN_RSA_KEY_OPSZ(rsa_key)) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	if (digest_length < digestsz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Copy the message digest to workmem. */
	memcpy(workmem, digest, digestsz);

	sx_pk_req req;
	struct sx_pk_slot inputs[6];
	int input_sizes[INPUT_SLOTS] = {0};

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, rsa_key->cmd);

	cracen_ffkey_write_sz(rsa_key, input_sizes);
	CRACEN_FFKEY_REFER_INPUT(rsa_key, input_sizes) = signature->sz;
	status = sx_pk_list_gfp_inslots(&req, input_sizes, inputs);
	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	cracen_ffkey_write(rsa_key, inputs);
	sx_wrpkmem(CRACEN_FFKEY_REFER_INPUT(rsa_key, inputs).addr, signature->r, signature->sz);

	/* start the modular exponentiation s^e mod n (RSAVP1 verify primitive)
	 */
	sx_pk_run(&req);
	status = sx_pk_wait(&req);
	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	size_t paddingstrsz;
	size_t dersz;

	/* get reference DER encoding of the hash algorithm ID */
	const uint8_t *refder = get_hash_der(hashalg, &dersz);

	if (dersz == 0) {
		sx_pk_release_req(&req);
		return SX_ERR_UNSUPPORTED_HASH_ALG;
	}

	/* set pointer to encoded message (EM in RFC 8017), assumed big endian
	 */
	const uint8_t *const encodedmsgstart = outputs[0];

	/* set pointer to the first octet beyond the encoded message */
	const uint8_t *const encodedmsgend = encodedmsgstart + modulussz;

	/* pointer used to walk through the EM */
	const uint8_t *encmsg = (const uint8_t *)encodedmsgstart;

	/* invalid signature if first two bytes of EM are not 0x00 0x01 */
	r = (sx_rdpkmem_byte(encmsg) != 0x00);
	encmsg++;

	r |= (sx_rdpkmem_byte(encmsg) != 0x1);
	encmsg++;

	/* check the presence of the padding string (PS in RFC 8017) */
	while ((encmsg < encodedmsgend) && (sx_rdpkmem_byte(encmsg) == 0xFF)) {
		encmsg++;
	}

	/* compute size of the padding string PS */
	paddingstrsz = (size_t)(encmsg - encodedmsgstart) - HEADER_BYTES;

	/* The size of the PS must match the empty space in the EM. This ensures
	 * that no other information follows the hash value in the EM. This check is
	 * required by FIPS 186-4, section 5.5, point f.
	 */
	if (paddingstrsz != (modulussz - 3 - dersz - digestsz)) {
		sx_pk_release_req(&req);
		return SX_ERR_INVALID_SIGNATURE;
	}

	/* there must be a zero byte separating PS from T */
	r |= (sx_rdpkmem_byte(encmsg) != 0x00);
	encmsg++;

	/* check the DER encoding of the hash algorithm ID (first part of T) */
	r |= constant_memcmp(encmsg, refder, dersz);
	encmsg += dersz;

	/* check the hash part of T: reference hash value is in workmem */
	r |= constant_memcmp(encmsg, workmem, digestsz);

	sx_pk_release_req(&req);

	if (r == 0) {
		return SX_OK;
	} else {
		return SX_ERR_INVALID_SIGNATURE;
	}
}
