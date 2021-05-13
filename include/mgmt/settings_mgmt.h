
/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file settings_mgmt.h
 * @defgroup settings_mgmt MCUMgr module for managing settings
 * @{
 * @brief MCUMgr based Settings management.
 *
 * Module for managing settings area with the SMP protocol.
 *
 * The command values are 6 for erase.
 *
 */


#ifndef SETTINGS_MGMT_H__
#define SETTINGS_MGMT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Setup and register the command handler for settings management
 *  command handler group.
 *
 *  Since settings mgmt uses the same group ID as OS mgmt it needs to be
 *  initialized before OS mgmt.
 */

void settings_mgmt_init(void);

/** @brief Setup and register the command handler for settings management
 *  command handler group.
 *
 *  Since settings mgmt uses the same group ID as OS mgmt it needs to be
 *  initialized before OS mgmt.
 */

int storage_erase(void);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* SETTINGS_MGMT_H__ */
