/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cracen_psa.h>
#include "sxsymcrypt/internal.h"

#include <stddef.h>
#include <stdint.h>
#include <silexpk/core.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>

/* Size of the workmem of the MGF1XOR function. */
#define MGF1XOR_WORKMEM_SZ(digestsz) ((digestsz) + 4)

/* Return a pointer to the part of workmem that is specific to RSA. */
static inline uint8_t *cracen_get_rsa_workmem_pointer(uint8_t *workmem, size_t digestsz)
{
	return workmem + MGF1XOR_WORKMEM_SZ(digestsz);
}

static inline size_t cracen_get_rsa_workmem_size(size_t workmemsz, size_t digestsz)
{
	return workmemsz - MGF1XOR_WORKMEM_SZ(digestsz);
}

enum asn1_tags {
	ASN1_SEQUENCE = 0x10,
	ASN1_INTEGER = 0x2,
	ASN1_CONSTRUCTED = 0x20,
	ASN1_BIT_STRING = 0x3
};

/**
 * \brief Tries to extract an RSA key from ASN.1.
 *
 * \param[out] rsa                 Resulting RSA key.
 * \param[in]  extract_pubkey      true to extract public key. false to extract private key.
 * \param[in]  is_key_pair         false if key is a public key. true if it is an RSA key pair.
 * \param[in]  key                 Buffer containing RSA key in ASN.1 format.
 * \param[in]  keylen              Length of buffer in bytes.
 * \param[out] modulus             Modulus (n) operand of n.
 * \param[out] exponent            Public or private exponent, depending on \p extract_pubkey.
 *
 * \return sxsymcrypt status code.
 */
int cracen_signature_get_rsa_key(struct cracen_rsa_key *rsa, bool extract_pubkey, bool is_key_pair,
				 const uint8_t *key, size_t keylen, struct sx_buf *modulus,
				 struct sx_buf *exponent);

/**
 * \brief Extracts an ASN.1 integer (excluding the leading zeros) and sets up an sx_buff pointing to
 * it.
 *
 * \param[in,out]  p               ASN.1 buffer, will be updated to point to the next ASN.1 element.
 * \param[in]      end             The end of the ASN.1 buffer.
 * \param[out]     op              The sx_buff operand which will hold the pointer to the extracted
 * ASN.1 integer.
 *
 * \retval SX_OK on success.
 * \retval SX_ERR_INVALID_PARAM if the ASN.1 integer cannot be extracted.
 */
int cracen_signature_asn1_get_operand(const uint8_t **p, const uint8_t *end, struct sx_buf *op);

/** Modular exponentiation (base^key mod n).
 *
 * This function is used by both the sign and the verify functions. Note: if the
 * base is greater than the modulus, SilexPK will return the SX_ERR_OUT_OF_RANGE
 * status code.
 */
/** Caller must call sx_pk_acquire_hw(req) before and sx_pk_release_req(req) after. */
int cracen_rsa_modexp(sx_pk_req *req, struct sx_pk_slot *inputs,
		      struct cracen_rsa_key *rsa_key, const uint8_t *base, size_t basez,
		      int *sizes);

#define CRACEN_KEY_INIT_RSA(mod, expon)								   \
	(struct cracen_rsa_key)									   \
	{											   \
		.cmd = SX_PK_CMD_MOD_EXP, .slotmask = (1 | (1 << 2)), .dataidx = 1,		   \
		{										   \
			mod, expon								   \
		}										   \
	}

/** Initialize an RSA CRT key consisting of 2 primes and derived values.
 *
 * The 2 primes (p and q) must comply with the rules laid out in
 * the latest version of FIPS 186. This includes that the 2 primes must
 * have the highest bit set in their most significant byte. The full
 * key size in bits must be a multiple of 8.
 */
#define CRACEN_KEY_INIT_RSACRT(p, q, dp, dq, qinv)						   \
	(struct cracen_rsa_key)									   \
	{											   \
		.cmd = SX_PK_CMD_MOD_EXP_CRT, .slotmask = (0x3 | (0x7 << 3)), .dataidx = 2,	   \
		{										   \
			p, q, dp, dq, qinv							   \
		}										   \
	}
