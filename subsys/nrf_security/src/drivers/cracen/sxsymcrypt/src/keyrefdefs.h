/** Key reference common defines used by AES, SM4 and AEAD.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KEYREFDEFS_HEADER_FILE
#define KEYREFDEFS_HEADER_FILE

#include <stddef.h>

/* Used for validating key reference */
#define KEYREF_IS_INVALID(k) (((k) == NULL) || (((k)->key == NULL) && ((k)->cfg == 0)))

/* Used for checking if \p k is user provided key */
#define KEYREF_IS_USR(k) ((k)->cfg == 0)

/** BA411E-AES Config register -> KeySel[1:0] = [7:6], KeySel[4:2] = [30:28] */
#define KEYREF_BA411E_HWKEY_CONF(index) ((((index)&0x3) << 6) | ((((index) >> 2) & 0x7) << 28))

#endif
