/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_BOOT_H_
#define BL_BOOT_H_

#include <fw_info.h>

/** Prepare the device and boot the firmware.
 *
 *  Before booting, the function does necessary cleanup and preparation, such
 *  as uninitializing UART and setting VTOR.
 *
 *  @param[in] fw_info  The firmware's fw_info struct. The pointer must point
 *                      to the actual structure in flash, not a copy.
 *
 *  @return  If validation fails this function returns, otherwise it transfers
 *           control to the firmware and doesn't return.
 */
void bl_boot(const struct fw_info *fw_info);

#endif /* BL_BOOT_H_ */
