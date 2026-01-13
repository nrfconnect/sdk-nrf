/* Encryption scheme RSAES-PKCS1-v1_5, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSAES-PKCS1-v1_5 encrypt and decrypt functions:
 *      1. buffer of k bytes, where k is the size in bytes of the RSA modulus.
 *
 * In the encrypt function, the workmem area is first used to form the encoded
 * message, then to store the encrypted message.
 * In the decrypt function, the workmem area is used to store the result of the
 * modular exponentiation, i.e. the encoded message.
 */

#include <internal/rsa/cracen_rsa_common.h>
#include <internal/rsa/cracen_rsa_key.h>
#include <internal/rsa/cracen_rsa_signature_pkcs1v15.h>

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen_psa_primitives.h>

#define WORKMEM_SIZE	PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS)
#define HEADER_BYTES	2
#define NUMBER_OF_SLOTS 6

int cracen_rsa_pkcs1v15_decrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length)
{
	int sx_status;
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	uint8_t workmem[WORKMEM_SIZE];

	if (modulussz > WORKMEM_SIZE) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	if (modulussz <= 11) {
		return SX_ERR_INVALID_ARG;
	}

	/* check ciphertext size (step 1 of RSAES-PKCS1-V1_5-DECRYPT modified:
	 * we allow the ciphertext to have length <= than the modulus length)
	 */
	if (text->sz > modulussz) {
		return SX_ERR_TOO_BIG;
	}
	int input_sizes[NUMBER_OF_SLOTS];
	sx_pk_req req;
	struct sx_pk_slot inputs[NUMBER_OF_SLOTS];

	sx_pk_acquire_hw(&req);
	sx_status = cracen_rsa_modexp(&req, inputs, rsa_key, text->addr, text->sz, input_sizes);
	if (sx_status != SX_OK) {
		sx_pk_release_req(&req);
		return sx_status;
	}
	int r = 0;
	uint8_t *paddingstr;
	size_t paddingstrsz;

	/* copy output of exponentiation (the encoded message EM) to workmem */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(workmem, outputs[0], opsz);
	sx_pk_release_req(&req);

	/*  pointer to the first octet after the encoded message */
	const uint8_t *encodedmsgend = workmem + opsz;

	r |= (workmem[0] != 0x00);
	r |= (workmem[1] != 0x02);

	/* scan the padding string and find the 0x00 octet that marks it end */
	paddingstr = workmem + HEADER_BYTES;
	while ((paddingstr < encodedmsgend) && (*paddingstr != 0)) {
		paddingstr++;
	}

	r |= (paddingstr == encodedmsgend); /* 0x00 octet not found */

	paddingstrsz = (size_t)(paddingstr - workmem) - HEADER_BYTES;
	r |= (paddingstrsz < 8); /* padding string must be at least 8 bytes long */

	/* step 3 of RSAES-PKCS1-V1_5-DECRYPT in RFC 8017 */
	if (r != 0) {
		return SX_ERR_INVALID_CIPHERTEXT;
	}

	/* update the cracen_crypt_text structure to point to the decrypted message */
	text->addr = paddingstr + 1;
	text->sz = encodedmsgend - text->addr;

	memcpy(output, text->addr, encodedmsgend - text->addr);
	*output_length = encodedmsgend - text->addr;
	safe_memzero(workmem, sizeof(workmem));
	return SX_OK;
}

int cracen_rsa_pkcs1v15_encrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length)
{
	int sx_status;
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	uint8_t workmem[WORKMEM_SIZE];

	if (modulussz > WORKMEM_SIZE) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	if (modulussz <= 11) {
		return SX_ERR_INVALID_ARG;
	}

	/* step 1 of RSAES-PKCS1-V1_5-ENCRYPT */
	if (text->sz > modulussz - 11) {
		return SX_ERR_TOO_BIG;
	}

	size_t msgsz = text->sz;
	size_t paddingstrsz = modulussz - msgsz - 3;

	/* the encoded message is being formed in workmem */
	workmem[0] = 0x00;
	workmem[1] = 0x02;
	workmem[modulussz - msgsz - 1] = 0x00;

	/* copy plaintext message to workmem */
	memcpy(workmem + modulussz - msgsz, text->addr, msgsz);

	/* The counter of PRNG requests is initialized to its max allowed value:
	 * 100 times the size of the padding string PS. The retry counter has type
	 * unsigned long because, assuming as worst case RSA key size a 8192-bit key,
	 * the worst case PS size is 1021 bytes, so the worst case number of allowed
	 * retries is 102100. This number cannot be represented as unsigned int if the
	 * machine has 16-bit int, while it can always be represented as unsigned long
	 * (the C standard specifies type long be at least 32 bits wide).
	 */
	unsigned long retryctr = 100 * paddingstrsz;
	unsigned int zeros;
	psa_status_t psa_status;
	uint8_t *psleft;
	uint8_t *psright;

	/* const pointers to the start and the end of the padding string */
	uint8_t *paddingstrstart = workmem + 2;
	uint8_t *const paddingstrend = paddingstrstart + paddingstrsz;

	/* padding string (PS in RFC 8017): ask for random bytes */
	do {
		zeros = 0;
		/* decrement counter of PRNG requests */
		retryctr--;

		psa_status = cracen_get_random(NULL, paddingstrstart, paddingstrsz);
		if (psa_status != PSA_SUCCESS) {
			safe_memzero(workmem, sizeof(workmem));
			return SX_ERR_UNKNOWN_ERROR;
		}

		psleft = paddingstrstart;
		psright = paddingstrend - 1;

		/* if there are zero octets, move them to the end of the padding string
		 */
		while (psleft <= psright) {
			if (*psleft == 0x00) {
				if (*psright == 0x00) {
					psright--;
				} else {
					*psleft = *psright;
					*psright = 0x00;
					psright--;
					psleft++;
				}
				zeros++;
			} else {
				psleft++;
			}
		}
		paddingstrstart = paddingstrend - zeros;
		paddingstrsz = paddingstrend - paddingstrstart;
	} while (retryctr > 0 && zeros > 0);
	if (retryctr == 0) {
		return SX_ERR_TOO_MANY_ATTEMPTS;
	}

	int input_sizes[NUMBER_OF_SLOTS];
	sx_pk_req req;
	struct sx_pk_slot inputs[NUMBER_OF_SLOTS];

	sx_pk_acquire_hw(&req);
	/* proceed to modular exponentiation if no zero octets found */
	sx_status = cracen_rsa_modexp(&req, inputs, rsa_key, workmem, modulussz, input_sizes);
	if (sx_status != SX_OK) {
		safe_memzero(workmem, sizeof(workmem));
		sx_pk_release_req(&req);
		return sx_status;
	}

	/* get result of modular exponentiation, placing it in workmem */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(workmem, outputs[0], opsz);
	sx_pk_release_req(&req);

	memcpy(output, workmem, opsz);
	*output_length = opsz;
	safe_memzero(workmem, sizeof(workmem));

	return SX_OK;
}
