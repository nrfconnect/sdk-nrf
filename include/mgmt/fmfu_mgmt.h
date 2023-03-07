/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file fmfu_mgmt.h
 * @defgroup fmfu_mgmt MCUMgr module for full Modem Firmware Upgrade
 * @{
 * @brief MCUMgr based Full Modem Firmware Update(FMFU).
 *
 * The Full Modem Firmware Upgrade (FMFU) is a module with functions
 * that hooks into MCUMgr for doing a full modem update over a transport layer
 * with the SMP protocol.
 *
 * The command values are 0 for get_hash and 1 for firmware_upload
 *
 */

#ifndef FMFU_MGMT_H__
#define FMFU_MGMT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Setup and register the command handler for full modem
 *  update command handler group.
 *
 * The modem library must be initialized in DFU mode before calling this
 * function - nrf_modem_lib_init(BOOTLOADER_MODE).
 *
 * @retval 0 on success, negative integer on failure.
 */
int fmfu_mgmt_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FMFU_MGMT_H__ */
