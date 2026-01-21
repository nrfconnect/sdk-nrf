/* Isolated Key Generation (IKG) operations
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the sign operation:
 *      1. Hash digest of the message to be signed (size: digestsz).
 *
 * Workmem layout for the sign digest operation:
 *      1. Hash digest of the message to be signed (size: digestsz).
 *
 * Other IKG functions don't need workmem memory.
 */

#include <stdint.h>
#include <string.h>
#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ik.h>
#include <cracen/statuscodes.h>
#include "cracen_ikg.h"
#include <cracen_psa_primitives.h>
#include <cracen/common.h>

#define MAX_ATTEMPTS 255

/** Convert a digest into an operand for ECDSA
 *
 * The raw digest may need to be padded or truncated to fit the curve
 * operand used for ECDSA.
 *
 * Conversion could also imply byte order inversion, but that is not
 * implemented here. It's expected that SilexPK takes big endian
 * operands here.
 *
 * As the digest size is expressed in bytes, this procedure does not
 * work for curves which have sizes not multiples of 8 bits.
 */
static void digest2op(const uint8_t *digest, size_t sz, uint8_t *dst, size_t opsz)
{
	if (opsz > sz) {
		sx_clrpkmem(dst, opsz - sz);
		dst += opsz - sz;
	}
	sx_wrpkmem(dst, digest, opsz);
}

static void ecdsa_read_sig(struct cracen_signature *sig, const uint8_t *r, const uint8_t *s,
			   size_t opsz)
{
	sx_rdpkmem(sig->r, r, opsz);
	if (!sig->s) {
		sig->s = sig->r + opsz;
	}
	sx_rdpkmem(sig->s, s, opsz);
}

static int exit_ikg(sx_pk_req *req)
{
	sx_pk_set_cmd(req, SX_PK_CMD_IK_EXIT);
	int status = sx_pk_list_ik_inslots(req, 0, NULL);

	if (status != SX_OK) {
		return status;
	}
	sx_pk_run(req);
	return sx_pk_wait(req);
}

int cracen_ikg_sign_message(int identity_key_index, const struct sxhashalg *hashalg,
			    const struct sx_pk_ecurve *curve, const uint8_t *message,
			    size_t message_length, uint8_t *signature)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_ikg_sign_digest(identity_key_index, hashalg, curve, digest, digestsz,
				      signature);
}

int cracen_ikg_sign_digest(int identity_key_index, const struct sxhashalg *hashalg,
			   const struct sx_pk_ecurve *curve, const uint8_t *digest,
			   size_t digest_length, uint8_t *signature)
{
	int status = SX_ERR_RETRY;
	size_t opsz = sx_pk_curve_opsize(curve);
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ik_ecdsa_sign inputs;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t workmem[digestsz];
	struct cracen_signature internal_signature = {0};

	memcpy(workmem, digest, digest_length);

	internal_signature.r = signature;
	internal_signature.s = signature + opsz;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_ECDSA_SIGN);
	if (pkreq.status) {
		sx_pk_release_req(pkreq.req);
		return pkreq.status;
	}

	for (int i = 0; status != SX_OK && i <= MAX_ATTEMPTS; i++) {
		if (i > 0) {
			sx_pk_set_cmd(pkreq.req, SX_PK_CMD_IK_ECDSA_SIGN);
		}

		pkreq.status = sx_pk_list_ik_inslots(pkreq.req, identity_key_index,
						     (struct sx_pk_slot *)&inputs);
		if (pkreq.status) {
			sx_pk_release_req(pkreq.req);
			return pkreq.status;
		}

		digest2op(workmem, digestsz, inputs.h.addr, opsz);
		sx_pk_run(pkreq.req);
		status = sx_pk_wait(pkreq.req);

		/* SX_ERR_NOT_INVERTIBLE may be due to silexpk countermeasures. */
		if (status == SX_ERR_RETRY ||
		    status == SX_ERR_INVALID_SIGNATURE ||
		    status == SX_ERR_NOT_INVERTIBLE) {
			int exit_status = exit_ikg(pkreq.req);

			if (exit_status != SX_OK) {
				sx_pk_release_req(pkreq.req);
				return exit_status;
			}

			if (i == MAX_ATTEMPTS) {
				sx_pk_release_req(pkreq.req);
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
			status = SX_ERR_RETRY;
		}
	}

	if (status != SX_OK) {
		sx_pk_release_req(pkreq.req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);

	ecdsa_read_sig(&internal_signature, outputs[0], outputs[1], opsz);
	safe_memzero(workmem, digestsz);

	status = exit_ikg(pkreq.req);
	sx_pk_release_req(pkreq.req);
	return status;
}

int cracen_ikg_create_pub_key(int identity_key_index, uint8_t *pub_key)
{
	int status = SX_ERR_RETRY;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_PUBKEY_GEN);
	if (pkreq.status) {
		sx_pk_release_req(pkreq.req);
		return pkreq.status;
	}

	for (int i = 0; status != SX_OK && i <= MAX_ATTEMPTS; i++) {
		if (i > 0) {
			sx_pk_set_cmd(pkreq.req, SX_PK_CMD_IK_PUBKEY_GEN);
		}

		pkreq.status = sx_pk_list_ik_inslots(pkreq.req, identity_key_index, NULL);
		if (pkreq.status) {
			sx_pk_release_req(pkreq.req);
			return pkreq.status;
		}

		sx_pk_run(pkreq.req);
		status = sx_pk_wait(pkreq.req);

		if (status == SX_ERR_RETRY) {
			int exit_status = exit_ikg(pkreq.req);

			if (exit_status != SX_OK) {
				sx_pk_release_req(pkreq.req);
				return exit_status;
			}

			if (i == MAX_ATTEMPTS) {
				sx_pk_release_req(pkreq.req);
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
		}
	}

	if (status != SX_OK) {
		sx_pk_release_req(pkreq.req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(pub_key, outputs[0], opsz);
	sx_rdpkmem(pub_key + opsz, outputs[1], opsz);

	status = exit_ikg(pkreq.req);
	sx_pk_release_req(pkreq.req);
	return status;
}

static void cracen_set_ikg_key_buffer(psa_key_attributes_t *attributes,
				      psa_drv_slot_number_t slot_number, uint8_t *key_buffer)
{
	ikg_opaque_key *ikg_key = (ikg_opaque_key *)key_buffer;

	switch (slot_number) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		/* The slot_number is not used with the identity key */
		break;
	case CRACEN_BUILTIN_MKEK_ID:
		ikg_key->slot_number = CRACEN_INTERNAL_HW_KEY1_ID;
		break;
	case CRACEN_BUILTIN_MEXT_ID:
		ikg_key->slot_number = CRACEN_INTERNAL_HW_KEY2_ID;
		break;
	}

	ikg_key->owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
}

psa_status_t cracen_ikg_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes,
					uint8_t *key_buffer, size_t key_buffer_size,
					size_t *key_buffer_length)
{
	size_t opaque_key_size;
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;

	/* According to the PSA Crypto Driver specification, the PSA core will set the `id`
	 * and the `lifetime` field of the attribute struct. We will fill all the other
	 * attributes, and update the `lifetime` field to be more specific.
	 */
	switch (slot_number) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 CRACEN_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(attributes, 256);
		psa_set_key_algorithm(attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_SIGN_MESSAGE |
							    PSA_KEY_USAGE_SIGN_HASH |
							    PSA_KEY_USAGE_VERIFY_HASH |
							    PSA_KEY_USAGE_VERIFY_MESSAGE);

		status = cracen_get_opaque_size(attributes, &opaque_key_size);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* According to the PSA Crypto Driver interface proposed document the driver
		 * should fill the attributes even if the buffer of the key is too small. So
		 * we check the buffer here and not earlier in the function.
		 */
		if (key_buffer_size >= opaque_key_size) {
			*key_buffer_length = opaque_key_size;
			cracen_set_ikg_key_buffer(attributes, slot_number, key_buffer);
			return PSA_SUCCESS;
		} else {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		break;

	case CRACEN_BUILTIN_MKEK_ID:
	case CRACEN_BUILTIN_MEXT_ID:
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 CRACEN_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(attributes, PSA_KEY_TYPE_AES);
		psa_set_key_bits(attributes, 256);
		psa_set_key_algorithm(attributes, PSA_ALG_SP800_108_COUNTER_CMAC);
		psa_set_key_usage_flags(attributes,
					PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_VERIFY_DERIVATION);

		status = cracen_get_opaque_size(attributes, &opaque_key_size);
		if (status != PSA_SUCCESS) {
			return status;
		}
		/* See comment about the placement of this check in the previous switch
		 * case.
		 */
		if (key_buffer_size >= opaque_key_size) {
			*key_buffer_length = opaque_key_size;
			cracen_set_ikg_key_buffer(attributes, slot_number, key_buffer);
			return PSA_SUCCESS;
		} else {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}
