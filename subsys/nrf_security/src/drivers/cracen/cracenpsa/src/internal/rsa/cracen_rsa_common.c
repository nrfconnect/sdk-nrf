/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/rsa/cracen_rsa_common.h>
#include <internal/rsa/cracen_rsa_key.h>

#include <stddef.h>
#include <cracen/statuscodes.h>
#include <silexpk/core.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/cmddefs/rsa.h>

#define NOT_ENABLED_HASH_ALG (0)

static const uint8_t RSA_ALGORITHM_IDENTIFIER[] = {0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
						   0x0d, 0x01, 0x01, 0x01, 0x05, 0x00};

static int cracen_asn1_get_len(const uint8_t **p, const uint8_t *end, size_t *len)
{
	if ((end - *p) < 1) {
		return SX_ERR_INVALID_PARAM;
	}

	if ((**p & 0x80) == 0) {
		*len = *(*p)++;
	} else {
		int n = (**p) & 0x7F;

		if (n == 0 || n > 4) {
			return SX_ERR_INVALID_PARAM;
		}
		if ((end - *p) <= n) {
			return SX_ERR_INVALID_PARAM;
		}
		*len = 0;
		(*p)++;
		while (n--) {
			*len = (*len << 8) | **p;
			(*p)++;
		}
	}

	if (*len > (size_t)(end - *p)) {
		return SX_ERR_INVALID_PARAM;
	}

	return 0;
}

static int cracen_asn1_get_tag(const uint8_t **p, const uint8_t *end, size_t *len, int tag)
{
	if ((end - *p) < 1) {
		return SX_ERR_INVALID_PARAM;
	}

	if (**p != tag) {
		return SX_ERR_INVALID_PARAM;
	}

	(*p)++;

	return cracen_asn1_get_len(p, end, len);
}

static int cracen_asn1_get_int(const uint8_t **p, const uint8_t *end, int *val)
{
	int ret = SX_ERR_INVALID_PARAM;
	size_t len;

	ret = cracen_asn1_get_tag(p, end, &len, ASN1_INTEGER);
	if (ret != 0) {
		return ret;
	}

	if (len == 0) {
		return SX_ERR_INVALID_PARAM;
	}
	/* Reject negative integers. */
	if ((**p & 0x80) != 0) {
		return SX_ERR_INVALID_PARAM;
	}

	/* Skip leading zeros. */
	while (len > 0 && **p == 0) {
		++(*p);
		--len;
	}

	/* Reject integers that don't fit in an int. This code assumes that
	 * the int type has no padding bit.
	 */
	if (len > sizeof(int)) {
		return SX_ERR_INVALID_PARAM;
	}
	if (len == sizeof(int) && (**p & 0x80) != 0) {
		return SX_ERR_INVALID_PARAM;
	}

	*val = 0;
	while (len-- > 0) {
		*val = (*val << 8) | **p;
		(*p)++;
	}

	return 0;
}

int cracen_signature_asn1_get_operand(const uint8_t **p, const uint8_t *end,
				      struct sx_buf *op)
{
	int ret;
	size_t len = 0;
	size_t i = 0;

	ret = cracen_asn1_get_tag(p, end, &len, ASN1_INTEGER);
	if (ret) {
		return SX_ERR_INVALID_PARAM;
	}

	if (*p + len > end) {
		return SX_ERR_INVALID_PARAM;
	}

	/* Drop starting zeros, if any */
	for (i = 0; i < len; i++) {
		if ((*p)[i] != 0) {
			break;
		}
	}
	op->bytes = (uint8_t *)(*p + i);
	op->sz = len - i;

	*p += len;

	return SX_OK;
}

int cracen_signature_get_rsa_key(struct cracen_rsa_key *rsa, bool extract_pubkey, bool is_key_pair,
				 const uint8_t *key, size_t keylen, struct sx_buf *modulus,
				 struct sx_buf *exponent)
{
	int ret, version;
	size_t len;
	const uint8_t *parser_ptr;
	const uint8_t *end;

	parser_ptr = key;
	end = parser_ptr + keylen;

	if (!extract_pubkey && !is_key_pair) {
		return SX_ERR_INVALID_KEYREF;
	}

	/*
	 * This function parses the RSA keys (PKCS#1)
	 *
	 *  RSAPrivateKey ::= SEQUENCE {
	 *      version           Version,
	 *      modulus           INTEGER,  -- n
	 *      publicExponent    INTEGER,  -- e
	 *      privateExponent   INTEGER,  -- d
	 *      prime1            INTEGER,  -- p
	 *      prime2            INTEGER,  -- q
	 *      exponent1         INTEGER,  -- d mod (p-1)
	 *      exponent2         INTEGER,  -- d mod (q-1)
	 *      coefficient       INTEGER,  -- (inverse of q) mod p
	 *      otherPrimeInfos   OtherPrimeInfos OPTIONAL
	 *  }
	 *
	 *  RSAPublicKey ::= SEQUENCE {
	 *      version           Version,
	 *      modulus           INTEGER,  -- n
	 *      publicExponent    INTEGER,  -- e
	 *  }
	 *
	 *  OpenSSL wraps public keys with an RSA algorithm identifier that we skip
	 *  if it is present.
	 */
	ret = cracen_asn1_get_tag(&parser_ptr, end, &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE);
	if (ret) {
		return SX_ERR_INVALID_KEYREF;
	}

	end = parser_ptr + len;

	if (is_key_pair) {
		ret = cracen_asn1_get_int(&parser_ptr, end, &version);
		if (ret) {
			return SX_ERR_INVALID_KEYREF;
		}
		if (version != 0) {
			return SX_ERR_INVALID_KEYREF;
		}
	} else {
		/* Skip algorithm identifier prefix. */
		const uint8_t *id_seq = parser_ptr;

		ret = cracen_asn1_get_tag(&id_seq, end, &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE);
		if (ret == 0) {
			if (len != sizeof(RSA_ALGORITHM_IDENTIFIER)) {
				return SX_ERR_INVALID_KEYREF;
			}

			if (memcmp(id_seq, RSA_ALGORITHM_IDENTIFIER, len) != 0) {
				return SX_ERR_INVALID_KEYREF;
			}

			id_seq += len;

			ret = cracen_asn1_get_tag(&id_seq, end, &len, ASN1_BIT_STRING);
			if (ret != 0 || *id_seq != 0) {
				return SX_ERR_INVALID_KEYREF;
			}

			parser_ptr = id_seq + 1;

			ret = cracen_asn1_get_tag(&parser_ptr, end, &len,
						  ASN1_CONSTRUCTED | ASN1_SEQUENCE);
			if (ret) {
				return SX_ERR_INVALID_KEYREF;
			}
		}
	}

	*rsa = CRACEN_KEY_INIT_RSA(modulus, exponent);

	/* Import N */
	ret = cracen_signature_asn1_get_operand(&parser_ptr, end, modulus);
	if (ret) {
		return ret;
	}

	if (PSA_BYTES_TO_BITS(modulus->sz) > PSA_MAX_KEY_BITS) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* Import E */
	ret = cracen_signature_asn1_get_operand(&parser_ptr, end, exponent);
	if (ret) {
		return ret;
	}
	if (extract_pubkey) {
		return SX_OK;
	}

	/* Import D */
	ret = cracen_signature_asn1_get_operand(&parser_ptr, end, exponent);
	if (ret) {
		return ret;
	}

	return SX_OK;
}

int cracen_rsa_modexp(sx_pk_req *req, struct sx_pk_slot *inputs,
		      struct cracen_rsa_key *rsa_key, const uint8_t *base, size_t basez, int *sizes)
{
	int status;

	sx_pk_set_cmd(req, rsa_key->cmd);
	cracen_ffkey_write_sz(rsa_key, sizes);
	CRACEN_FFKEY_REFER_INPUT(rsa_key, sizes) = basez;
	status = sx_pk_list_gfp_inslots(req, sizes, inputs);
	if (status != SX_OK) {
		return status;
	}

	/* copy modulus and exponent to device memory */
	cracen_ffkey_write(rsa_key, inputs);
	sx_wrpkmem(CRACEN_FFKEY_REFER_INPUT(rsa_key, inputs).addr, base, basez);

	sx_pk_run(req);
	return sx_pk_wait(req);
}
