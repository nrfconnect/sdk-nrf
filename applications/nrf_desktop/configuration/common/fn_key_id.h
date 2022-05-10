/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FN_KEY_ID_H_
#define _FN_KEY_ID_H_

#include <zephyr/sys/util.h>
#include <caf/key_id.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _FN_POS		(_COL_POS + _COL_SIZE)
#define _FN_BIT		BIT(_FN_POS)

#define FN_KEY_ID(_col, _row)	(KEY_ID(_col, _row) | _FN_BIT)
#define IS_FN_KEY(_keyid)	((_FN_BIT & _keyid) != 0)

#ifdef __cplusplus
}
#endif

#endif /* _FN_KEY_ID_H_ */
