/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TFM_FWU_BOOTLOADER_DEFS_H
#define TFM_FWU_BOOTLOADER_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* FWU message types that distinguish FWU services. */
#define TFM_FWU_START                1001
#define TFM_FWU_WRITE                1002
#define TFM_FWU_FINISH               1003
#define TFM_FWU_CANCEL               1004
#define TFM_FWU_INSTALL              1005
#define TFM_FWU_CLEAN                1006
#define TFM_FWU_REJECT               1007
#define TFM_FWU_REQUEST_REBOOT       1008
#define TFM_FWU_ACCEPT               1009
#define TFM_FWU_QUERY                1010

#ifdef __cplusplus
}
#endif
#endif /* TFM_FWU_BOOTLOADER_DEFS_H */
