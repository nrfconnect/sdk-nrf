/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef SRC_INCLUDE_OTDOA_FILE_H_
#define SRC_INCLUDE_OTDOA_FILE_H_

#if CONFIG_OTDOA_LITTLE_FS
int mount_little_fs(void);
int unmount_little_fs(void);
int test_little_fs(void);
#else
int mount_fat_fs(void);
int test_fs_raw(void);
int test_fs(void);
#endif

#endif /* SRC_INCLUDE_OTDOA_FILE_H_ */
