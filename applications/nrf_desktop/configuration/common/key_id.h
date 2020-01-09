/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _KEY_ID_H_
#define _KEY_ID_H_

#include <zephyr/types.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _ROW_POS  0ul
#define _ROW_SIZE 7ul
#define _COL_POS  (_ROW_POS + _ROW_SIZE)
#define _COL_SIZE 7ul
#define _FN_POS   (_COL_POS + _COL_SIZE)

#define _FN_BIT         BIT(_FN_POS)
#define _COL_BITS(_col) ((_col & BIT_MASK(_COL_SIZE)) << _COL_POS)
#define _ROW_BITS(_row) ((_row & BIT_MASK(_ROW_SIZE)) << _ROW_POS)


#define KEY_ID(_col, _row) ((u16_t)(_COL_BITS(_col) | _ROW_BITS(_row)))

#define KEY_COL(_keyid) ((_keyid >> _COL_POS) & BIT_MASK(_COL_SIZE))
#define KEY_ROW(_keyid) ((_keyid >> _ROW_POS) & BIT_MASK(_ROW_SIZE))

#if defined(CONFIG_DESKTOP_FN_KEYS_ENABLE)
#define FN_KEY_ID(_col, _row) (KEY_ID(_col, _row) | _FN_BIT)
#define IS_FN_KEY(_keyid) ((_FN_BIT & _keyid) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _KEY_ID_H_ */
