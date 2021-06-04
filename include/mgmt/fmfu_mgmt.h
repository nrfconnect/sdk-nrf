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

enum fmfu_evt_id {
	/** Prepare for full modem update to start. */
	FMFU_EVT_PRE_DFU_MODE,
	/** Full modem update finished. */
	FMFU_EVT_FINISHED,
	/** Full modem update failed. */
	FMFU_EVT_ERROR,
};

/**
 * @brief FMFU event callback function.
 *
 * @param event_id Event ID.
 *
 */
typedef void (*fmfu_callback_t)(enum fmfu_evt_id id);

/** @brief Setup and register the command handler for full modem
 *  update command handler group.
 *
 * The modem library must be initialized in DFU mode when a FMFU_EVT_INIT event
 * occurs in the callback by calling this function -
 * nrf_modem_lib_init(FULL_DFU_MODE).
 *
 * @param client_callback Callback for handling events when FMFU is triggered.
 *
 * @retval 0 on success, negative integer on failure.
 */
int fmfu_mgmt_register_group(fmfu_callback_t client_callback);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FMFU_MGMT_H__ */
