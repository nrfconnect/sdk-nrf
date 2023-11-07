/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file mcumgr_smp_client.h
 *
 * @defgroup mcumgr_smp_client A client for downloading a firmware update over SMP
 * @{
 * @brief  Library for downloading a firmware update by download client and update that.
 *
 * @details Downloads the specified file from the specified host to the
 * secondary partition of MCUboot. After the file has been downloaded,
 * the secondary slot is tagged as having valid firmware inside it.
 */

#ifndef MCUMGR_SMP_CLIENT_H_
#define MCUMGR_SMP_CLIENT_H_
#include <dfu/dfu_target_smp.h>
#include <net/fota_download.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initialize DFU target SMP, MCUmgr client, and SMP client.
 *
 * @param cb Callback for resetting the device to enter MCUboot recovery mode.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_init(dfu_target_reset_cb_t cb);

/**@brief Start a manual download of the image for SMP server.
 *
 * @param download_uri Download uri for download image.
 * @param sec_tag Used sec tag for secure connection (HTTPs or CoAPs).
 * @param client_callback Callback for monitoring download process progressing.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_download_start(const char *download_uri, int sec_tag,
				     fota_download_callback_t client_callback);

/**@brief Cancel active image download.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_download_cancel(void);

/**@brief Activate downloaded image at next reboot.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_update(void);

/**@brief Read server image list.
 *
 * @param image_list Pointer for store data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_read_list(struct mcumgr_image_state *image_list);

/**@brief Reset device and exit recovery mode.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_reset(void);

/**@brief Erase SMP server secondary slot.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_erase(void);

/**@brief Confirm swapped image.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int mcumgr_smp_client_confirm_image(void);

#ifdef __cplusplus
}
#endif

#endif /* MCUMGR_SMP_CLIENT_H_ */

/**
 *@}
 */
