/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_rsa_key_management.h"
#include "cracen_rsa_keygen.h"

#include <string.h>
#include <silexpk/core.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <zephyr/sys/byteorder.h>
#include "cracen_rsa_common.h"

static psa_status_t check_rsa_key_attributes(const psa_key_attributes_t *attributes,
					     size_t key_bits)
{
	psa_algorithm_t key_alg = psa_get_key_algorithm(attributes);

	if (!PSA_ALG_IS_RSA_PKCS1V15_SIGN(key_alg) && !PSA_ALG_IS_RSA_PSS(key_alg) &&
	    !PSA_ALG_IS_RSA_OAEP(key_alg) && (key_alg != PSA_ALG_RSA_PKCS1V15_CRYPT)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	switch (key_bits) {
	case 1024:
		return PSA_SUCCESS;
	case 2048:
		return PSA_SUCCESS;
	case 3072:
		return PSA_SUCCESS;
	case 4096:
		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

static size_t get_asn1_size_with_tag_and_length(size_t sz)
{
	size_t r = 2 + sz; /* 1 byte tag, 1 byte for size and buffer size. */

	/* If size >= 0x80 we need additional bytes. */
	if (sz >= 0x80) {
		while (sz) {
			r += 1;
			sz >>= 8;
		}
	}

	return r;
}

static void write_tag_and_length(struct sx_buf *buf, uint8_t tag)
{
	size_t length = buf->sz;
	uint8_t *outbuf = buf->bytes - get_asn1_size_with_tag_and_length(buf->sz) + length;

	*outbuf++ = tag;
	if (length < 0x80) {
		*outbuf = length;
		return;
	}

	uint8_t len_bytes = get_asn1_size_with_tag_and_length(buf->sz) - length - 2;
	*outbuf++ = 0x80 | len_bytes;

	/* Write out length as big endian. */
	length = sys_cpu_to_be32(length);
	memcpy(outbuf, ((uint8_t *)&length) + sizeof(length) - len_bytes, len_bytes);
}

psa_status_t export_rsa_public_key_from_keypair(const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer,
						size_t key_buffer_size, uint8_t *data,
						size_t data_size, size_t *data_length)
{
	/*
	 * RSAPublicKey ::= SEQUENCE {
	 * modulus            INTEGER,    -- n
	 * publicExponent     INTEGER  }  -- e
	 */

	size_t key_bits_attr = psa_get_key_bits(attributes);
	struct cracen_rsa_key rsa_key;
	struct sx_buf n = {0};
	struct sx_buf e = {0};

	psa_status_t status = check_rsa_key_attributes(attributes, key_bits_attr);

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = silex_statuscodes_to_psa(cracen_signature_get_rsa_key(
		&rsa_key, true, true, key_buffer, key_buffer_size, &n, &e));

	if (status != PSA_SUCCESS) {
		return status;
	}

	struct sx_buf sequence = {0};

	/* Array of buffers in the order they will be serialized. */
	struct sx_buf *buffers[] = {&n, &e};

	for (size_t i = 0; i < ARRAY_SIZE(buffers); i++) {
		if (buffers[i]->bytes[0] & 0x80) {
			buffers[i]->sz++;
		}
		sequence.sz += get_asn1_size_with_tag_and_length(buffers[i]->sz);
	}

	size_t total_size = get_asn1_size_with_tag_and_length(sequence.sz);

	sequence.bytes = data + total_size - sequence.sz;

	if (total_size > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Start placing buffers from the end */
	size_t offset = total_size;

	for (int i = ARRAY_SIZE(buffers) - 1; i >= 0; i--) {
		uint8_t *new_location = data + offset - buffers[i]->sz;

		if (buffers[i]->bytes[0] & 0x80) {
			*new_location = 0x0;
			memcpy(new_location + 1, buffers[i]->bytes, buffers[i]->sz - 1);
		} else {
			memcpy(new_location, buffers[i]->bytes, buffers[i]->sz);
		}
		buffers[i]->bytes = new_location;
		offset -= get_asn1_size_with_tag_and_length(buffers[i]->sz);
		write_tag_and_length(buffers[i], ASN1_INTEGER);
	}

	write_tag_and_length(&sequence, ASN1_SEQUENCE | ASN1_CONSTRUCTED);

	*data_length = total_size;
	return PSA_SUCCESS;
}

psa_status_t generate_rsa_private_key(const psa_key_attributes_t *attributes, uint8_t *key,
				      size_t key_size, size_t *key_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	size_t bits = psa_get_key_bits(attributes);
	size_t key_size_bytes = PSA_BITS_TO_BYTES(bits);
	size_t key_size_half = key_size_bytes / 2;
	int sx_status;

	/* RSA public exponent used in PSA is 65537. We provide this as a 3 byte
	 * big endian array.
	 */
	uint8_t pub_exponent[] = {0x01, 0x00, 0x01};

	psa_status_t status = check_rsa_key_attributes(attributes, bits);

	if (status != PSA_SUCCESS) {
		return status;
	}

	/*
	 * Setup RSA Private key layout.
	 *
	 * RSAPrivateKey ::= SEQUENCE {
	 *		version INTEGER, -- must be 0
	 *		modulus INTEGER, -- n
	 *		publicExponent INTEGER, -- e
	 *		privateExponent INTEGER, -- d
	 *		prime1 INTEGER, -- p
	 *		prime2 INTEGER, -- q
	 *		exponent1 INTEGER, -- d mod (p-1)
	 *		exponent2 INTEGER, -- d mod (q-1)
	 *		coefficient INTEGER, -- (inverse of q) mod p
	 * }
	 */
	uint8_t version_bytes = 0;
	struct sx_buf sequence = {.sz = 0};
	struct sx_buf version = {.bytes = &version_bytes, .sz = sizeof(version_bytes)};

	/* The buffers are first laid out sequentially in the output buffer
	 * by the caller. When the key generation is finished we place the
	 * buffers correctly and write the ASN.1 tag and size fields.
	 */
	struct sx_buf n = {.bytes = key, .sz = key_size_bytes};
	struct sx_buf e = {.bytes = pub_exponent, .sz = sizeof(pub_exponent)};
	struct sx_buf d = {.bytes = n.bytes + n.sz, .sz = key_size_bytes};
	struct sx_buf p = {.bytes = d.bytes + d.sz, .sz = key_size_half};
	struct sx_buf q = {.bytes = p.bytes + p.sz, .sz = key_size_half};
	struct sx_buf dp = {.bytes = q.bytes + q.sz, .sz = key_size_half};
	struct sx_buf dq = {.bytes = dp.bytes + dp.sz, .sz = key_size_half};
	struct sx_buf qinv = {.bytes = dq.bytes + dq.sz, .sz = key_size_half};

	/* Array of buffers in the order they will be serialized. */
	struct sx_buf *buffers[] = {&version, &n, &e, &d, &p, &q, &dp, &dq, &qinv};

	if ((uint8_t *)(qinv.bytes + qinv.sz) > (key + key_size)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Generate RSA CRT key. */
	struct cracen_rsa_key privkey = CRACEN_KEY_INIT_RSACRT(&p, &q, &dp, &dq, &qinv);

	sx_status = cracen_rsa_generate_privkey(pub_exponent, sizeof(pub_exponent), key_size_bytes,
						&privkey);
	if (sx_status != SX_OK) {
		status = silex_statuscodes_to_psa(sx_status);
		goto error_exit;
	}

	/* Generate n and d */
	sx_const_op c_p;
	sx_const_op c_q;
	sx_const_op c_e;

	sx_get_const_op(&p, &c_p);
	sx_get_const_op(&q, &c_q);
	sx_get_const_op(&e, &c_e);
	status = silex_statuscodes_to_psa(sx_rsa_keygen(&c_p, &c_q, &c_e, &n, NULL, &d));
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	/* In DER encoding all numbers are in 2's complement form. We need to
	 * pad numbers where the first bit is set with 0x0 to encode them as
	 * positive.
	 */

	for (int i = ARRAY_SIZE(buffers) - 1; i >= 0; i--) {
		size_t sign_padding = 0;

		if (buffers[i]->bytes[0] & 0x80) {
			sign_padding = 1;
		}

		sequence.sz += get_asn1_size_with_tag_and_length(buffers[i]->sz + sign_padding);
	}

	size_t total_size = get_asn1_size_with_tag_and_length(sequence.sz);
	size_t offset = 0;

	if (total_size > key_size) {
		status = PSA_ERROR_BUFFER_TOO_SMALL;
		goto error_exit;
	}

	/* Place the buffers from the end and write ASN.1 tag and size fields.
	 */
	for (int i = ARRAY_SIZE(buffers) - 1; i >= 0; i--) {
		uint8_t *new_position = key + total_size - offset - buffers[i]->sz;

		memmove(new_position, buffers[i]->bytes, buffers[i]->sz);
		buffers[i]->bytes = new_position;

		if (buffers[i]->bytes[0] & 0x80) {
			buffers[i]->bytes--;
			buffers[i]->bytes[0] = 0;
			buffers[i]->sz++;
		}

		offset += get_asn1_size_with_tag_and_length(buffers[i]->sz);
		write_tag_and_length(buffers[i], ASN1_INTEGER);
	}

	sequence.bytes = key + total_size - offset;
	write_tag_and_length(&sequence, ASN1_CONSTRUCTED | ASN1_SEQUENCE);

	*key_length = total_size;

	return status;

error_exit:
	*key_length = 0;
	safe_memzero(key, key_size);

	return status;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0*/
}

psa_status_t import_rsa_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			    size_t *key_buffer_length, size_t *key_bits)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_key_type_t key_type = psa_get_key_type(attributes);
	bool is_public_key = key_type == PSA_KEY_TYPE_RSA_PUBLIC_KEY;

	struct cracen_rsa_key rsakey;
	struct sx_buf n = {0};
	struct sx_buf e = {0};

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* We copy the key to the internal buffer first, then validate it. */
	memcpy(key_buffer, data, data_length);

	psa_status_t status = silex_statuscodes_to_psa(cracen_signature_get_rsa_key(
		&rsakey, is_public_key, key_type == PSA_KEY_TYPE_RSA_KEY_PAIR, key_buffer,
		data_length, &n, &e));

	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	/* When importing keys the PSA APIs allow for key bits to be 0 and they
	 * expect it to be calculated based on the buffer size of the data. For
	 * RSA keys the key size is the size of the modulus.
	 */
	if (key_bits_attr == 0) {
		key_bits_attr = PSA_BYTES_TO_BITS(n.sz);
	}

	status = check_rsa_key_attributes(attributes, key_bits_attr);
	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	*key_buffer_length = data_length;
	*key_bits = key_bits_attr;

	return PSA_SUCCESS;

cleanup:
	*key_buffer_length = 0;
	*key_bits = 0;
	safe_memzero(key_buffer, key_buffer_size);
	return status;
}

psa_status_t rsa_export_public_key(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   uint8_t *data, size_t data_size, size_t *data_length)
{
	if (data_size < key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (data == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status_t status = check_rsa_key_attributes(attributes, psa_get_key_bits(attributes));

	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(data, key_buffer, key_buffer_size);
	*data_length = key_buffer_size;

	return PSA_SUCCESS;
}
