/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NDEF_FILE_M_H__
#define _NDEF_FILE_M_H__

/** @file
 *
 * @defgroup nfc_writable_ndef_msg_example_ndef_file_m ndef_file_m.h
 * @{
 * @ingroup nfc_writable_ndef_msg_example
 * @brief FLASH service for the NFC writable NDEF message example.
 *
 */

#include <zephyr/types.h>

/**
 * @brief   Function for initializing the NVS module.
 *
 * @return 0 when module has been set up properly,
 * error code otherwise.
 */
int ndef_file_setup(void);

/**
 * @brief   Function for updating NDEF message in the flash file.
 *
 * @details NVS update operation is performed on main loop.
 *
 * @param buff Pointer to the NDEF message to be stored in flash.
 * @param size Size of NDEF message.
 *
 * @return  0 when update request has been added to the queue.
 *          Otherwise, error code.
 */
int ndef_file_update(uint8_t const *buff, uint32_t size);

/**
 * @brief Function for loading NDEF message from the flash file.
 *
 * @details If the record does not exist, the default NDEF message
 * is created and stored in flash.
 *
 * @param buff Pointer to the buffer for NDEF message.
 * @param size Size of the buffer.
 *
 * @return  0 when NDEF message has been retrieved properly. Otherwise error
 * code is returned.
 */
int ndef_file_load(uint8_t *buff, uint32_t size);

/**
 * @brief Function for creating the default NDEF message: URL "nordicsemi.com".
 *
 * @param buff Pointer to the NDEF message buffer.
 * @param size Pointer to the variable holding buffer size as input, size
 * of the created message as output.
 *
 * @return 0 if the message has been created, error code otherwise.
 */
int ndef_file_default_message(uint8_t *buff, uint32_t *size);

/**
 * @brief Function for creating and storing the default NDEF message:
 * URL "nordicsemi.com".
 *
 * @param buff Pointer to the NDEF message buffer.
 * @param size Pointer to the variable holding buffer size as input, size
 * of the created message as output.
 *
 * @return 0 if the message has been created, error code otherwise.
 */
int ndef_restore_default(uint8_t *buff, uint32_t size);

/** @} */

#endif /* _NDEF_FILE_M_H__ */
