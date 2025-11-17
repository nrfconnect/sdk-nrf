/* Isolated key operations (signature generation, ...)
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
#include <cracen_psa_ikg.h>
#include <cracen_psa_primitives.h>
#include "common.h"

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

static int exit_ikg(struct sx_pk_acq_req *pkreq)
{

	int status;

	*pkreq = sx_pk_acquire_req_locked(SX_PK_CMD_IK_EXIT);
	pkreq->status = sx_pk_list_ik_inslots(pkreq->req, 0, NULL);
	if (pkreq->status) {
		return pkreq->status;
	}
	sx_pk_run(pkreq->req);
	status = sx_pk_wait(pkreq->req);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_release_req(pkreq->req);
	return SX_OK;
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
	const uint8_t *curve_n;
	uint8_t workmem[digestsz];
	struct cracen_signature internal_signature = {0};

	memcpy(workmem, digest, digest_length);
	curve_n = sx_pk_curve_order(curve);

	internal_signature.r = signature;
	internal_signature.s = signature + opsz;
	int i = 0;

	while (status != SX_OK && i <= MAX_ATTEMPTS) {
		i++;
		pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_ECDSA_SIGN);
		if (pkreq.status) {
			return pkreq.status;
		}
		pkreq.status = sx_pk_list_ik_inslots(pkreq.req, identity_key_index,
						    (struct sx_pk_slot *)&inputs);
		if (pkreq.status) {
			return pkreq.status;
		}

		digest2op(workmem, digestsz, inputs.h.addr, opsz);
		sx_pk_run(pkreq.req);
		status = sx_pk_wait(pkreq.req);
		if (status == SX_ERR_RETRY) {
			status = exit_ikg(&pkreq);
			if (status != SX_OK) {
				return status;
			}
			status = SX_ERR_RETRY;
		}

		/* SX_ERR_NOT_INVERTIBLE may be due to silexpk countermeasures. */
		if ((status == SX_ERR_INVALID_SIGNATURE) || (status == SX_ERR_NOT_INVERTIBLE)) {
			sx_pk_release_req(pkreq.req);
			if (i == MAX_ATTEMPTS) {
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
		}
	}
	if (status != SX_OK) {
		sx_pk_release_req(pkreq.req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);

	ecdsa_read_sig(&internal_signature, outputs[0], outputs[1], opsz);
	safe_memzero(workmem, digestsz);

	return exit_ikg(&pkreq);
}

int cracen_ikg_create_pub_key(int identity_key_index, uint8_t *pub_key)
{
	int status = SX_ERR_RETRY;
	struct sx_pk_acq_req pkreq;
	int i = 0;

	/* The use of MAX_ATTEMPTS here is as an arbitrary value.
	 * Needed to have a max value for this while loop and that was already defined.
	 */
	while (status != SX_OK && i <= MAX_ATTEMPTS) {
		i++;
		pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_PUBKEY_GEN);
		if (pkreq.status) {
			return pkreq.status;
		}
		pkreq.status = sx_pk_list_ik_inslots(pkreq.req, identity_key_index, NULL);
		if (pkreq.status) {
			return pkreq.status;
		}

		sx_pk_run(pkreq.req);
		status = sx_pk_wait(pkreq.req);
		if (status == SX_ERR_RETRY) {
			status = exit_ikg(&pkreq);
			if (status != SX_OK) {
				return status;
			}
			status = SX_ERR_RETRY;
		}
	}
	if (status != SX_OK) {
		return status;
	}
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(pub_key, outputs[0], opsz);
	sx_rdpkmem(pub_key + opsz, outputs[1], opsz);
	return exit_ikg(&pkreq);
}
