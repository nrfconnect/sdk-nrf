/** Signature generation and verification and public key generation.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <cracen/statuscodes.h>
#include "../include/sicrypto/sig.h"
#include "../include/sicrypto/ecdsa.h"
#include "../include/sicrypto/internal.h"
#include "sigdefs.h"
#include "final.h"

void si_sig_create_sign(struct sitask *t, const struct si_sig_privkey *privkey,
			struct si_sig_signature *signature)
{
	assert(privkey->def);
	privkey->def->sign(t, privkey, signature);
}

void si_sig_create_sign_digest(struct sitask *t, const struct si_sig_privkey *privkey,
			       struct si_sig_signature *signature)
{
	assert(privkey->def);
	if (!privkey->def->sign_digest) {
		si_task_mark_final(t, SX_ERR_NOT_IMPLEMENTED);
		return;
	}
	privkey->def->sign_digest(t, privkey, signature);
}

void si_sig_create_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
			  const struct si_sig_signature *signature)
{
	assert(pubkey->def);
	if (!pubkey->def->verify) {
		si_task_mark_final(t, SX_ERR_NOT_IMPLEMENTED);
		return;
	}
	pubkey->def->verify(t, pubkey, signature);
}

void si_sig_create_verify_digest(struct sitask *t, const struct si_sig_pubkey *pubkey,
				 const struct si_sig_signature *signature)
{
	assert(pubkey->def);
	if (!pubkey->def->verify_digest) {
		si_task_mark_final(t, SX_ERR_NOT_IMPLEMENTED);
		return;
	}
	pubkey->def->verify_digest(t, pubkey, signature);
}

void si_sig_create_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
			  struct si_sig_pubkey *pubkey)
{
	assert(privkey->def);
	if (!privkey->def->pubkey) {
		si_task_mark_final(t, SX_ERR_NOT_IMPLEMENTED);
		return;
	}
	pubkey->def = privkey->def;
	privkey->def->pubkey(t, privkey, pubkey);
}

size_t si_sig_get_total_sig_size(const struct si_sig_privkey *privkey)
{
	return privkey->def->getkeyopsz(privkey) * privkey->def->sigcomponents;
}
