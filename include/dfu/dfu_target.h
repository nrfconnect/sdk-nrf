/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief DFU image type.
 *
 * Bitmasks of different image types.
 */
enum dfu_target_image_type {
	/** Not a DFU image */
	DFU_TARGET_IMAGE_TYPE_NONE = 0,
	/** Application image in MCUBoot format */
	DFU_TARGET_IMAGE_TYPE_MCUBOOT = 1,
	/** Modem delta-update image */
	DFU_TARGET_IMAGE_TYPE_MODEM_DELTA = 2,
	/** Full update image for modem */
	DFU_TARGET_IMAGE_TYPE_FULL_MODEM = 4,
	/** SMP external MCU */
	DFU_TARGET_IMAGE_TYPE_SMP = 8,
	/** Any application image type */
	DFU_TARGET_IMAGE_TYPE_ANY_APPLICATION = DFU_TARGET_IMAGE_TYPE_MCUBOOT,
	/** Any modem image */
	DFU_TARGET_IMAGE_TYPE_ANY_MODEM =
		(DFU_TARGET_IMAGE_TYPE_MODEM_DELTA | DFU_TARGET_IMAGE_TYPE_FULL_MODEM),
	/** Any DFU image type */
	DFU_TARGET_IMAGE_TYPE_ANY =
		(DFU_TARGET_IMAGE_TYPE_MCUBOOT | DFU_TARGET_IMAGE_TYPE_MODEM_DELTA |
		 DFU_TARGET_IMAGE_TYPE_FULL_MODEM),
};

enum dfu_target_evt_id {
	DFU_TARGET_EVT_TIMEOUT,
	DFU_TARGET_EVT_ERASE_DONE
};

typedef void (*dfu_target_callback_t)(enum dfu_target_evt_id evt_id);

/** @brief Functions which needs to be supported by all DFU targets.
 */
struct dfu_target {
	int (*init)(size_t file_size, int img_num, dfu_target_callback_t cb);
	int (*offset_get)(size_t *offset);
	int (*write)(const void *const buf, size_t len);
	int (*done)(bool successful);
	int (*schedule_update)(int img_num);
	int (*reset)();
};

/**
 * @brief Find the image type for the buffer of bytes received. Used to determine
 *	  what DFU target to initialize.
 *
 * @param[in] buf A buffer of bytes which are the start of a binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return Identifier for a supported image type or DFU_TARGET_IMAGE_TYPE_NONE if
 *         image type is not recognized.
 **/
enum dfu_target_image_type dfu_target_img_type(const void *const buf, size_t len);

/**
 * @brief Find the image type for the buffer of bytes received. Used to validate type for
 *	  DFU SMP target to initialize.
 *
 * @param[in] buf A buffer of bytes which are the start of a binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return DFU_TARGET_IMAGE_TYPE_SMP or DFU_TARGET_IMAGE_TYPE_NONE if
 *         image type is not recognized.
 **/
enum dfu_target_image_type dfu_target_smp_img_type_check(const void *const buf, size_t len);

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
 * @param[in] img_num Image pair index. Value different than 0 are supported
 *		      only for DFU_TARGET_IMAGE_TYPE_MCUBOOT image type
 *		      currently.
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] cb Callback function in case the DFU operation requires additional
 *		 procedures to be called.
 *
 * @return 0 for a supported image type or a negative error
 *	   code indicating reason of failure.
 *
 **/
int dfu_target_init(int img_type, int img_num, size_t file_size, dfu_target_callback_t cb);

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
 * @param[in] buf A buffer of bytes which contains part of a binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return Positive identifier for a supported image type or a negative error
 *	   code indicating reason of failure.
 **/
int dfu_target_write(const void *const buf, size_t len);

/**
 * @brief Release the resources that were needed for the current DFU
 *	  target.
 *
 * @param[in] successful Indicate whether the process completed successfully or
 *			 was aborted.
 *
 * @return 0 for a successful deinitialization or a negative error
 *	   code indicating reason of failure.
 **/
int dfu_target_done(bool successful);

/**
 * @brief Release the resources that were needed for the current DFU
 *	  target if any and resets the current DFU target.
 *
 * @return 0 for a successful deinitialization and reset or a negative error
 *	   code indicating reason of failure.
 **/
int dfu_target_reset(void);

/**
 * @brief Schedule update of one or more images.
 *
 * This call requests images update. Time of update depends on image type.
 *
 * @param[in] img_num For DFU_TARGET_IMAGE_TYPE_MCUBOOT
 *		      image type: given image pair index or -1 for all
 *		      of image pair indexes. Disregard otherwise.
 *
 * @return 0 for a successful request or a negative error
 *	   code indicating reason of failure.
 **/
int dfu_target_schedule_update(int img_num);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DFU_TARGET_H__ */
