/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_modem_delta.h
 *
 * @defgroup dfu_target_modem_delta Modem DFU Target
 * @{
 * @brief DFU Target for upgrades performed by Modem
 */

#ifndef DFU_TARGET_MODEM_H__
#define DFU_TARGET_MODEM_H__

#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Modem delta UUID length (ASCII). */
#define DFU_TARGET_MODEM_DELTA_UUID_LEN 36

/**
 * @brief See if data in buf indicates modem delta upgrade.
 *
 * @retval true if data matches, false otherwise.
 */
bool dfu_target_modem_delta_identify(const void *const buf);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] img_num Image pair index. The value is not used currently.
 * @param[in] callback Callback function for signaling if the modem is not able
 *		       to service the erase request.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_modem_delta_init(size_t file_size, int img_num,
				dfu_target_callback_t callback);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @retval 0 On success.
 * @retval -EAGAIN Scratch erase is still in progress.
 * @retval -EFAULT Other modem or internal failure.
 */
int dfu_target_modem_delta_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_modem_delta_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.
 *
 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_modem_delta_done(bool successful);

/**
 * @brief Schedule update of the image.
 *
 * This call requests image update. The update will be performed after
 * the device reset.
 *
 * @param[in] img_num This parameter is unused by this target type.
 *
 * @return 0 for a successful request or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_target_modem_delta_schedule_update(int img_num);

/**
 * @brief Release resources and erase the download area.
 *
 * Cancels any ongoing updates.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_modem_delta_reset(void);

/**
 * @brief Read the modem firmware UUID string.
 *
 * @param[out] uuid     Buffer for the UUID.
 * @param[in]  uuid_len Size of @p uuid (use @ref DFU_TARGET_MODEM_DELTA_UUID_LEN).
 *
 * @retval 0 On success.
 * @retval Negative errno on failure.
 */
int dfu_target_modem_delta_firmware_uuid(char *uuid, size_t uuid_len);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_MODEM_H__ */

/**@} */
