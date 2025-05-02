/* Encryption scheme RSAES-PKCS1-v1_5, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSAES-PKCS1-v1_5 encrypt and decrypt tasks:
 *      1. buffer of k bytes, where k is the size in bytes of the RSA modulus.
 *
 * In the encrypt task, the workmem area is first used to form the encoded
 * message, then to store the encrypted message.
 * In the decrypt task, the workmem area is used to store the result of the
 * modular exponentiation, i.e. the encoded message.
 * In both the encrypt and decrypt cases, the task's output, returned using
 * struct cracen_crypt_text *text, is inside the workmem area.
 */

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen_psa_rsa_signature_pkcs1v15.h>
#include <cracen_psa_primitives.h>
#include "common.h"
#include "rsakey.h"
#include "util.h"

int cracen_rsa_pkcs1v15_decrypt(struct cracen_rsa_key *rsa_key,
					struct si_ase_text *text)
{
	int sx_status;
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	size_t workmemsz = (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + PSA_HASH_MAX_SIZE + 4);
	uint8_t workmem[workmemsz];

	if (workmemsz < modulussz) {
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
	int sizes[6];
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	/* proceed to modular exponentiation if no zero octets found */
	sx_status = cracen_rsa_modexp(&pkreq, &inputs, &rsa_key, text->addr, text->sz, &sizes);
	if (sx_status != SX_OK) {
		return sx_status;
	}
	int r = 0;
	uint8_t *paddingstr;
	size_t paddingstrsz;

	/* copy output of exponentiation (the encoded message EM) to workmem */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(workmem, outputs[0], opsz);
	sx_pk_release_req(pkreq.req);

	/*  pointer to the first octet after the encoded message */
	uint8_t const *encodedmsgend = workmem + opsz;

	r |= (workmem[0] != 0x00);
	r |= (workmem[1] != 0x02);

	/* scan the padding string and find the 0x00 octet that marks it end */
	paddingstr = workmem + 2;
	while ((*paddingstr != 0) && (paddingstr < encodedmsgend)) {
		paddingstr++;
	}

	r |= (paddingstr == encodedmsgend); /* 0x00 octet not found */

	paddingstrsz = ((size_t)(paddingstr - workmem)) - 2;
	r |= (paddingstrsz < 8); /* padding string must be at least 8 bytes long */

	/* step 3 of RSAES-PKCS1-V1_5-DECRYPT in RFC 8017 */
	if (r != 0) {
		return SX_ERR_INVALID_CIPHERTEXT;
	}

	/* update the cracen_crypt_text structure to point to the decrypted message */
	text->addr = paddingstr + 1;
	text->sz = encodedmsgend - text->addr;

	return SX_OK;

}

int cracen_rsa_pkcs1v15_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_buf *label)
{
	int sx_status;
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	size_t workmemsz = (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + PSA_HASH_MAX_SIZE + 4);
	uint8_t workmem[workmemsz];

	if (workmemsz < modulussz) {
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
	unsigned int zeros = 0;
	psa_status_t psa_status;

	size_t modulussz = CRACEN_RSA_KEY_OPSZ(t->params.rsaes_pkcs1v15.key);
	size_t msgsz = text->sz;
	size_t paddingstrsz = modulussz - msgsz - 3;
	uint8_t *psleft;
	uint8_t *psright;

	/* const pointers to the start and the end of the padding string */
	uint8_t *const paddingstrstart = workmem + 2;
	uint8_t *const paddingstrend = paddingstrstart + paddingstrsz;

	/* padding string (PS in RFC 8017): ask for random bytes */
	do {
		/* decrement counter of PRNG requests */
		retryctr--;
		/* get random octets from the PRNG in the environment */
		psa_status = cracen_get_random(NULL, workmem + 2, paddingstrsz);
		if (psa_status != PSA_SUCCESS) {
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
	} while (retryctr > 0 && zeros > 0);
	if (retryctr == 0) {
		return SX_ERR_TOO_MANY_ATTEMPTS;
	}

	int sizes[6];
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	/* proceed to modular exponentiation if no zero octets found */
	sx_status = cracen_rsa_modexp(&pkreq, &inputs, rsa_key, &workmem, &modulussz, &sizes);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* get result of modular exponentiation, placing it in workmem */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(workmem, outputs[0], opsz);
	sx_pk_release_req(pkreq.req);

	/* update the cracen_crypt_text structure to point to the encrypted message */
	text->addr = t->workmem;
	text->sz = opsz;

	return SX_OK;
}
