/*
 * Copyright (c) 2017-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_PS_DEFS_H__
#define __TFM_PS_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Invalid UID */
#define TFM_PS_INVALID_UID 0

/* PS message types that distinguish PS services. */
#define TFM_PS_SET                1001
#define TFM_PS_GET                1002
#define TFM_PS_GET_INFO           1003
#define TFM_PS_REMOVE             1004
#define TFM_PS_GET_SUPPORT        1005

#ifdef __cplusplus
}
#endif

#endif /* __TFM_PS_DEFS_H__ */
