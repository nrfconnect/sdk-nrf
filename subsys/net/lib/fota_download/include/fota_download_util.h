/*
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_DOWNLOAD_UTIL_H__
#define FOTA_DOWNLOAD_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <net/fota_download.h>

/**
 * @brief Find correct MCUBoot update resource locator entry in space separated string.
 *
 * When supporting upgrades of the MCUBoot bootloader, two variants of the new firmware are
 * presented, each at a different address. This function is told which slot is active, and sets the
 * update pointer to point to the correct non-active entry in the dual resource locator.
 *
 * @param[in, out] file		 Pointer to dual resource locator with space separator. Note
 *				 that the space separator will be replaced by a NULL terminator.
 *
 * @param[in]      s0_active     Boolean indicating if S0 is the currently active slot.
 * @param[out]     selected_path Pointer to correct file MCUBoot bootloader upgrade. Will be set to
 *				 NULL if no space separator is found.
 *
 * @retval - 0 If successful (note that this does not imply that a space separator was found)
	   - negative errno otherwise.
 */
int fota_download_parse_dual_resource_locator(char *const file, bool s0_active,
					     const char **selected_path);


/**@brief Initialize internal FOTA download utils.
 *
 * @param client_callback Callback for monitoring download state.
 * @param smp_image_type Use true for DFU SMP target otherwise use false.

 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_client_init(fota_download_callback_t client_callback, bool smp_image_type);

/**@brief Initialize DFU stream targets.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_stream_init(void);

/**@brief Pre-initialize for selected DFU target
 *
 * @param dfu_target_type DFU target
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_dfu_target_init(enum dfu_target_image_type dfu_target_type);

/**@brief Start downloading the image for selected DFU type and download URL.
 *
 * @param download_uri Download URI string.
 * @param dfu_target_type DFU target type for new image
 * @param sec_tag Used security tag for HTTPs or CoAPs
 * @param client_callback Callback for monitoring download state
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_download_start(const char *download_uri,
				      enum dfu_target_image_type dfu_target_type, int sec_tag,
				      fota_download_callback_t client_callback);

/**@brief Cancel active download process.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_download_cancel(void);

/**@brief Schedule image update for next reset.
 *
 * @param dfu_target_type DFU target
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_image_schedule(enum dfu_target_image_type dfu_target_type);

/**@brief Reset DFU target FOTA state
 *
 * @param dfu_target_type DFU target
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_image_reset(enum dfu_target_image_type dfu_target_type);

/**@brief Activate secondary image slot.
 *
 * @param dfu_target_type DFU target
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_util_apply_update(enum dfu_target_image_type dfu_target_type);

#ifdef __cplusplus
}
#endif

#endif /*FOTA_DOWNLOAD_UTIL_H__ */
