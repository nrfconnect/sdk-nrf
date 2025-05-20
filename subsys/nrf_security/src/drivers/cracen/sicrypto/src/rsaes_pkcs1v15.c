/* Encryption scheme RSAES-PKCS1-v1_5, based on RFC 8017.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
 * struct si_ase_text *text, is inside the workmem area.
 */

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/rsaes_pkcs1v15.h"
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include "waitqueue.h"
#include "rsakey.h"
#include "final.h"
#include "util.h"

/* Modular exponentiation: base^key mod n */
static int rsaes_pkcs1v15_start_modexp(struct sitask *t, const char *base, size_t basesz)
{
	struct si_rsa_key *key = t->params.rsaes_pkcs1v15.key;
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	int sizes[6];

	pkreq = sx_pk_acquire_req(key->cmd);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	/* set up values in the sizes[] array */
	si_ffkey_write_sz(key, sizes);
	SI_FFKEY_REFER_INPUT(key, sizes) = basesz;

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, inputs);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	/* copy the key elements to device memory */
	si_ffkey_write(key, inputs);

	/* copy the base operand to device memory */
	sx_wrpkmem(SI_FFKEY_REFER_INPUT(key, inputs).addr, base, basesz);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	/* start the modular exponentiation */
	sx_pk_run(t->pk);

	return SX_ERR_HW_PROCESSING;
}

/* get result of exponentiation, perform checks and output decrypted message */
static int rsaes_pkcs1v15_decrypt_finish(struct sitask *t, struct siwq *wq)
{
	int r = 0;
	char *paddingstr;
	size_t paddingstrsz;
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* copy output of exponentiation (the encoded message EM) to workmem */
	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(t->workmem, outputs[0], opsz);
	sx_pk_release_req(t->pk);

	/*  pointer to the first octet after the encoded message */
	char const *encodedmsgend = t->workmem + opsz;

	r |= (t->workmem[0] != 0x00);
	r |= (t->workmem[1] != 0x02);

	/* scan the padding string and find the 0x00 octet that marks it end */
	paddingstr = t->workmem + 2;
	while ((*paddingstr != 0) && (paddingstr < encodedmsgend)) {
		paddingstr++;
	}

	r |= (paddingstr == encodedmsgend); /* 0x00 octet not found */

	paddingstrsz = ((size_t)(paddingstr - t->workmem)) - 2;
	r |= (paddingstrsz < 8); /* padding string must be at least 8 bytes long */

	/* step 3 of RSAES-PKCS1-V1_5-DECRYPT in RFC 8017 */
	if (r != 0) {
		return si_task_mark_final(t, SX_ERR_INVALID_CIPHERTEXT);
	}

	/* update the si_ase_text structure to point to the decrypted message */
	t->params.rsaes_pkcs1v15.text->addr = paddingstr + 1;
	t->params.rsaes_pkcs1v15.text->sz = encodedmsgend - t->params.rsaes_pkcs1v15.text->addr;

	return SX_OK;
}

static void rsaes_pkcs1v15_decrypt_run(struct sitask *t)
{
	si_wq_run_after(t, &t->params.rsaes_pkcs1v15.wq, rsaes_pkcs1v15_decrypt_finish);

	/* modular exponentiation c^d mod n (RSADP decryption primitive) */
	rsaes_pkcs1v15_start_modexp(t, t->params.rsaes_pkcs1v15.text->addr,
				    t->params.rsaes_pkcs1v15.text->sz);
}

void si_ase_create_rsa_pkcs1v15_decrypt(struct sitask *t, struct si_rsa_key *privkey,
					struct si_ase_text *text)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(privkey);

	if (t->workmemsz < modulussz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	/* check RSA modulus size (step 1 of RSAES-PKCS1-V1_5-DECRYPT) */
	if (modulussz < 11) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* check ciphertext size (step 1 of RSAES-PKCS1-V1_5-DECRYPT modified:
	 * we allow the ciphertext to have length <= than the modulus length)
	 */
	if (text->sz > modulussz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	t->params.rsaes_pkcs1v15.key = privkey;
	t->params.rsaes_pkcs1v15.text = text;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.run = rsaes_pkcs1v15_decrypt_run;
}

static int rsaes_pkcs1v15_encrypt_continue(struct sitask *t, struct siwq *wq);

/* get random octets from the PRNG in the environment */
static void rsaes_pkcs1v15_encrypt_getrnd(struct sitask *t, char *out, size_t sz)
{
	psa_status_t status = cracen_get_random(NULL, out, sz);

	if (status != PSA_SUCCESS) {
		si_task_mark_final(t, SX_ERR_UNKNOWN_ERROR);
		return;
	}

	si_wq_run_after(t, &t->params.rsaes_pkcs1v15.wq, rsaes_pkcs1v15_encrypt_continue);

	t->actions.status = si_status_on_prng;
	t->actions.wait = si_wait_on_prng;
}

static int rsaes_pkcs1v15_encrypt_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* get result of modular exponentiation, placing it in workmem */
	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(t->workmem, outputs[0], opsz);
	sx_pk_release_req(t->pk);

	/* update the si_ase_text structure to point to the encrypted message */
	t->params.rsaes_pkcs1v15.text->addr = t->workmem;
	t->params.rsaes_pkcs1v15.text->sz = opsz;

	return t->statuscode;
}

static int rsaes_pkcs1v15_encrypt_continue(struct sitask *t, struct siwq *wq)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaes_pkcs1v15.key);
	size_t msgsz = t->params.rsaes_pkcs1v15.text->sz;
	size_t paddingstrsz = modulussz - msgsz - 3;
	char *psleft, *psright;
	unsigned int zeros = 0;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* const pointers to the start and the end of the padding string */
	char *const paddingstrstart = t->workmem + 2;
	char *const paddingstrend = paddingstrstart + paddingstrsz;

	/* decrement counter of PRNG requests */
	t->params.rsaes_pkcs1v15.retryctr--;

	/* impose an upper bound on retries to ensure task does not loop forever
	 */
	if (t->params.rsaes_pkcs1v15.retryctr == 0) {
		return si_task_mark_final(t, SX_ERR_TOO_MANY_ATTEMPTS);
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

	if (zeros) {
		/* get more random bytes: final PS must not contain any zero
		 * octets
		 */
		rsaes_pkcs1v15_encrypt_getrnd(t, paddingstrend - zeros, zeros);
	} else {
		/* proceed to modular exponentiation if no zero octets found */
		rsaes_pkcs1v15_start_modexp(t, t->workmem, modulussz);

		si_wq_run_after(t, &t->params.rsaes_pkcs1v15.wq, rsaes_pkcs1v15_encrypt_finish);
	}

	t->statuscode = SX_ERR_HW_PROCESSING;
	return SX_ERR_HW_PROCESSING;
}

static void rsaes_pkcs1v15_encrypt_run(struct sitask *t)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaes_pkcs1v15.key);
	size_t msgsz = t->params.rsaes_pkcs1v15.text->sz;
	size_t paddingstrsz = modulussz - msgsz - 3;

	/* the encoded message is being formed in workmem */
	t->workmem[0] = 0x00;
	t->workmem[1] = 0x02;
	t->workmem[modulussz - msgsz - 1] = 0x00;

	/* copy plaintext message to workmem */
	memcpy(t->workmem + modulussz - msgsz, t->params.rsaes_pkcs1v15.text->addr, msgsz);

	/* padding string (PS in RFC 8017): ask for random bytes */
	rsaes_pkcs1v15_encrypt_getrnd(t, t->workmem + 2, paddingstrsz);

	/* The counter of PRNG requests is initialized to its max allowed value:
	 * 100 times the size of the padding string PS. The retry counter has type
	 * unsigned long because, assuming as worst case RSA key size a 8192-bit key,
	 * the worst case PS size is 1021 bytes, so the worst case number of allowed
	 * retries is 102100. This number cannot be represented as unsigned int if the
	 * machine has 16-bit int, while it can always be represented as unsigned long
	 * (the C standard specifies type long be at least 32 bits wide).
	 */
	t->params.rsaes_pkcs1v15.retryctr = 100 * paddingstrsz;
}

void si_ase_create_rsa_pkcs1v15_encrypt(struct sitask *t, struct si_rsa_key *pubkey,
					struct si_ase_text *text)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(pubkey);

	if (t->workmemsz < modulussz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	if (modulussz <= 11) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* step 1 of RSAES-PKCS1-V1_5-ENCRYPT */
	if (text->sz > modulussz - 11) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	t->params.rsaes_pkcs1v15.key = pubkey;
	t->params.rsaes_pkcs1v15.text = text;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.run = rsaes_pkcs1v15_encrypt_run;
}
