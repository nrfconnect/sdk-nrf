/*
 * Copyright (c) 2019-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_ITS_DEFS_H__
#define __TFM_ITS_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Invalid UID */
#define TFM_ITS_INVALID_UID 0

/* ITS message types that distinguish ITS services. */
#define TFM_ITS_SET                1001
#define TFM_ITS_GET                1002
#define TFM_ITS_GET_INFO           1003
#define TFM_ITS_REMOVE             1004

#ifdef __cplusplus
}
#endif

#endif /* __TFM_ITS_DEFS_H__ */
