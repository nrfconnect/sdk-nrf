/** Definitions for signature related keys.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SIGDEFS_HEADER_FILE
#define SIGDEFS_HEADER_FILE

#include "../include/sicrypto/sig.h"

typedef void (*signfunc)(struct sitask *t, const struct si_sig_privkey *privkey,
			 struct si_sig_signature *signature);
typedef void (*verifyfunc)(struct sitask *t, const struct si_sig_pubkey *pubkey,
			   const struct si_sig_signature *signature);
typedef void (*pubkeyfunc)(struct sitask *t, const struct si_sig_privkey *privkey,
			   struct si_sig_pubkey *pubkey);
typedef unsigned short int (*opszfunc)(const struct si_sig_privkey *privkey);

struct si_sig_def {
	signfunc sign;
	signfunc sign_digest;
	verifyfunc verify;
	verifyfunc verify_digest;
	pubkeyfunc pubkey;
	opszfunc getkeyopsz;
	unsigned short int sigcomponents;
};

#endif
