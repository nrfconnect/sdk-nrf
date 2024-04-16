/** Isolated key support.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_IK_HEADER_FILE
#define SICRYPTO_IK_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "sig.h"

/** Fetch an isolated private key. */
struct si_sig_privkey si_sig_fetch_ikprivkey(const struct sx_pk_ecurve *curve, int index);

#ifdef __cplusplus
}
#endif

#endif
