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

#include <internal/ikg/cracen_ikg_operations.h>

#include <stdint.h>
#include <string.h>
#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ik.h>
#include <cracen/statuscodes.h>
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
	sx_pk_req req;
	struct sx_pk_inops_ik_ecdsa_sign inputs;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t workmem[digestsz];
	struct cracen_signature internal_signature = {0};

	memcpy(workmem, digest, digest_length);

	internal_signature.r = signature;
	internal_signature.s = signature + opsz;

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_IK_ECDSA_SIGN);

	for (int i = 0; status != SX_OK && i <= MAX_ATTEMPTS; i++) {
		if (i > 0) {
			sx_pk_set_cmd(&req, SX_PK_CMD_IK_ECDSA_SIGN);
		}

		status = sx_pk_list_ik_inslots(&req, identity_key_index,
					       (struct sx_pk_slot *)&inputs);
		if (status != SX_OK) {
			sx_pk_release_req(&req);
			return status;
		}

		digest2op(workmem, digestsz, inputs.h.addr, opsz);
		sx_pk_run(&req);
		status = sx_pk_wait(&req);

		/* SX_ERR_NOT_INVERTIBLE may be due to silexpk countermeasures. */
		if (status == SX_ERR_RETRY ||
		    status == SX_ERR_INVALID_SIGNATURE ||
		    status == SX_ERR_NOT_INVERTIBLE) {
			int exit_status = exit_ikg(&req);

			if (exit_status != SX_OK) {
				sx_pk_release_req(&req);
				return exit_status;
			}

			if (i == MAX_ATTEMPTS) {
				sx_pk_release_req(&req);
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
			status = SX_ERR_RETRY;
		}
	}

	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);

	ecdsa_read_sig(&internal_signature, outputs[0], outputs[1], opsz);
	safe_memzero(workmem, digestsz);

	status = exit_ikg(&req);
	sx_pk_release_req(&req);
	return status;
}

int cracen_ikg_create_pub_key(int identity_key_index, uint8_t *pub_key)
{
	int status = SX_ERR_RETRY;
	sx_pk_req req;

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_IK_PUBKEY_GEN);

	for (int i = 0; status != SX_OK && i <= MAX_ATTEMPTS; i++) {
		if (i > 0) {
			sx_pk_set_cmd(&req, SX_PK_CMD_IK_PUBKEY_GEN);
		}

		status = sx_pk_list_ik_inslots(&req, identity_key_index, NULL);
		if (status != SX_OK) {
			sx_pk_release_req(&req);
			return status;
		}

		sx_pk_run(&req);
		status = sx_pk_wait(&req);

		if (status == SX_ERR_RETRY) {
			int exit_status = exit_ikg(&req);

			if (exit_status != SX_OK) {
				sx_pk_release_req(&req);
				return exit_status;
			}

			if (i == MAX_ATTEMPTS) {
				sx_pk_release_req(&req);
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
		}
	}

	if (status != SX_OK) {
		sx_pk_release_req(&req);
		return status;
	}

	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(pub_key, outputs[0], opsz);
	sx_rdpkmem(pub_key + opsz, outputs[1], opsz);

	status = exit_ikg(&req);
	sx_pk_release_req(&req);
	return status;
}
