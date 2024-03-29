/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_DFU_H__
#define SUIT_DFU_H__

/**
 * @brief Module providing tools to the Local domain Core allowing to orchestrate the SUIT DFU.
 */

/**
 * The following steps are required by the application to perform Device Firmware Update:
 *
 * (1) Call @ref suit_dfu_initialize to initialize the SUIT DFU module before any
 *     other SUIT functionalities are used. This function only has to be called once.
 *     This function must be called with at least APPLICATION init level (calling it
 *     in POST_KERNEL or earlier init level causes undefined behavior).
 * (2) Optional: If the SUIT manifest may require fetching of additional data in the
 *     suit-payload-fetch sequence, fetch sources should be registered using
 *     suit_fetch_source_register
 * (3) Store the SUIT candidate envelope in the non-volatile memory area defined as
 *     dfu_partition in the device tree.
 * (4) Optional, not supported yet: Store other needed data/detached payloads
 *     in dfu cache partitions.
 *     DFU cache pool 0 is not available at this stage (it is part of dfu_partition
 *     not occupied by the SUIT candidate envelope).
 * (5) When storing the SUIT candidate envelope is finished, call
 *     @ref suit_dfu_candidate_envelope_stored
 * (6) Call @ref suit_dfu_candidate_preprocess. If any fetch sources were registered
 *     in step (2) they might be used by this function to fetch additional data.
 * (7) Call @ref suit_dfu_update_start. This will start the firmware update by
 *     resetting the device and passing control over the update to the Secure Domain.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SUIT DFU before it can be used.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_dfu_initialize(void);

/**
 * @brief Purge all the SUIT DFU partitions and perform cleanup.
 *        Call this if the update needs to be interrupted after suit_dfu_initialize is called.
 *        This will invalidate any written data. To start the update again, SUIT DFU must
 *        be reinitialized with suit_dfu_envelope_size_update.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_dfu_cleanup(void);

/**
 * @brief Inform the SUIT module that the candidate envelope upload has ended.
 *
 * After this stage no modifications to the candidate are allowed (unless the API user
 * performs cleanup with @ref suit_dfu_cleanup and restarts the update process).
 * However, modifications to DFU cache partitions are still allowed.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_dfu_candidate_envelope_stored(void);

/**
 * @brief Process all the information stored in the envelope, but do not start the update yet.
 *        This function runs the SUIT processor on the envelope. If any fetch sources were
 *        registered using suit_fetch_source_register they might be used
 *        by the SUIT processor to pull any missing images. Also, cache partitions might be
 *        modified.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_dfu_candidate_preprocess(void);

/**
 * @brief Start the update.
 *        This will trigger a reset and pass the control to the Secure Domain in order to perform
 *        firmware update based on the installed envelope and other data stored in the SUIT DFU
 *        cache as a result of earlier operations.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_dfu_update_start(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_DFU_H__ */
