/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DFU_TARGET_H__
#define DFU_TARGET_H__

/**
 * @defgroup dfu_target DFU Target
 * @brief DFU Target
 *
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DFU_TARGET_IMAGE_TYPE_MCUBOOT 1
#define DFU_TARGET_IMAGE_TYPE_MODEM_DELTA 2

enum dfu_target_evt_id {
	DFU_TARGET_EVT_TIMEOUT,
	DFU_TARGET_EVT_ERASE_DONE
};

typedef void (*dfu_target_callback_t)(enum dfu_target_evt_id evt_id);

/** @brief Functions which needs to be supported by all DFU targets.
 */
struct dfu_target {
	int (*init)(size_t file_size, dfu_target_callback_t cb);
	int (*offset_get)(size_t *offset);
	int (*write)(const void *const buf, size_t len);
	int (*done)(bool successful);
};

/**
 * @brief Find the image type for the buffer of bytes recived. Used to determine
 *	  what dfu target to initialize.
 *
 * @param[in] buf A buffer of bytes which are the start of an binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return Positive identifier for a supported image type or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_target_img_type(const void *const buf, size_t len);

/**
 * @brief Initialize the resources needed for the specific image type DFU
 *	  target.
 *
 *	  If a target update is in progress, and the same target is
 *	  given as input, then calling the 'init()' function of that target is
 *	  skipped.
 *
 *	  To allow continuation of an aborted DFU procedure, call the
 *	  'dfu_target_offset_get' function after invoking this function.
 *
 * @param[in] img_type Image type identifier.
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] cb Callback function in case the DFU operation requires additional
 *		 proceedures to be called.
 *
 * @return 0 for a supported image type or a negative error
 *	   code identicating reason of failure.
 *
 **/
int dfu_target_init(int img_type, size_t file_size, dfu_target_callback_t cb);

/**
 * @brief Get offset of the firmware upgrade
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dfu_target_offset_get(size_t *offset);

/**
 * @brief Write the given buffer to the initialized DFU target.
 *
 * @param[in] buf A buffer of bytes which contains part of an binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return Positive identifier for a supported image type or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_target_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize the resources that were needed for the current DFU
 *	  target.
 *
 * @param[in] successful Indicate whether the process completed successfully or
 *			 was aborted.
 *
 * @return 0 for an successful deinitialization or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_target_done(bool successful);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_H__ */
