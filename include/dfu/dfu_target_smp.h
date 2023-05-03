/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_smp.h
 *
 * @defgroup dfu_target_SMP SMP external MCU target
 * @{
 * @brief DFU Target for upgrades performed by SMP Client.
 */

#ifndef DFU_TARGET_SMP_H__
#define DFU_TARGET_SMP_H__

#include <stddef.h>
#include <dfu/dfu_target.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DFU target reset callback for activating MCUboot serial recovery mode.
 *
 * @return 0 on success, negative errno otherwise.
 */
typedef int (*dfu_target_reset_cb_t)(void);

/**
 * @brief Register recovery mode reset callback.
 *
 * Function that boots the target in MCUboot serial recovery mode, needed before sending
 * SMP commands.
 *
 * @param[in] cb User defined target reset function for entering recovery mode.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_recovery_mode_enable(dfu_target_reset_cb_t cb);

/**
 * @brief Initialize dfu target SMP client.
 *
 * @retval 0 on success, negative errno otherwise.
 */
int dfu_target_smp_client_init(void);

/**
 * @brief Read image list.
 *
 * @param res_buf Image list buffer.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_image_list_get(struct mcumgr_image_state *res_buf);

/**
 * @brief Reboot SMP target device, and apply new image.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_reboot(void);

/**
 * @brief Confirm new image activation after reset command.
 *
 * Use only without MCUboot recovery mode.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_confirm_image(void);

/**
 * @brief Check if data in buffer indicates MCUboot style upgrade.
 *
 * @retval true if data matches, false otherwise.
 */
bool dfu_target_smp_identify(const void *const buf);

/**
 * @brief Initialize DFU target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] img_num Image pair index.
 * @param[in] cb Callback for signaling events (unused).
 *
 * @retval 0 on success, negative errno otherwise.
 */
int dfu_target_smp_init(size_t file_size, int img_num, dfu_target_callback_t cb);

/**
 * @brief Get offset of firmware.
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 on success, otherwise negative value if unable to get the offset.
 */
int dfu_target_smp_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.
 *
 * @param[in] successful Indicate whether the firmware was successfully received.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_done(bool successful);

/**
 * @brief Schedule update of image.
 *
 * This call requests image update. The update will be performed after
 * the device resets.
 *
 * @param[in] img_num Given image pair index or -1 for all
 *		      of image pair indexes.
 *
 * @return 0 for a successful request or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_target_smp_schedule_update(int img_num);

/**
 * @brief Release resources and erase the download area.
 *
 * Cancels any ongoing updates.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_smp_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_SMP_H__ */

/**@} */
