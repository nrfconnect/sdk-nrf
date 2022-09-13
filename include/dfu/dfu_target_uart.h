/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_TARGET_UART_H__
#define DFU_TARGET_UART_H__

#include <stddef.h>
#include <stdbool.h>
#include <dfu/dfu_target.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @def DFU_UART_HEADER_MAGIC
 *
 *  @brief UART target magic value. This value needs to be set in the envelope
 *  to select UART as active DFU type
 */
#define DFU_UART_HEADER_MAGIC               0x85f3d83a

/**
 * @struct dfu_target_uart_remote
 *
 * @brief Structure representing a single remote endpoint which can receive
 * application update
 */
struct dfu_target_uart_remote {
	/**
	 * Target identifier. This value shall be used in the cbor envelope
	 */
	uint32_t identifier;

	/**
	 * The uart device to use. Must not be NULL
	 */
	const struct device *device;
};

/**
 * @struct dfu_target_uart_params
 *
 * @brief Structure representing the configuration of the UART target type
 */
struct dfu_target_uart_params {
	/* The number of remotes in the configuration */
	size_t remote_count;
	/* The list of remotes to register */
	struct dfu_target_uart_remote *remotes;
	/* The output buffer to use */
	uint8_t *buffer;
	/* The output buffer size */
	size_t buf_size;
};

/**
 * @brief Configure resources required by dfu_target_uart
 *
 * @param[in] params Pointer to dfu type parameters.
 *
 * @note This function shall be called before any other function of
 * the target type.
 *
 * @return 0 on success, or -EINVAL if the configuration is invalid
 */
int dfu_target_uart_cfg(const struct dfu_target_uart_params *params);

/**
 * @brief See if data in buf indicates a UART type update.
 *
 * @param[in] buf Pointer to data to check.
 * @param[in] len Size of the input buffer.
 *
 * @retval true if data indicates a UART type upgrade, false otherwise.
 */
bool dfu_target_uart_identify(const void *buf, size_t len);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] img_num Image pair index. The value is not used currently.
 * @param[in] callback  Not in use. In place to be compatible with DFU target API.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_uart_init(size_t file_size, int img_num, dfu_target_callback_t callback);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 on success, otherwise negative value if unable to get the offset
 */
int dfu_target_uart_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_uart_write(const void *buf, size_t len);

/**
 * @brief De-initialize resources and finalize firmware upgrade if successful.

 * @param[in] successful Indicate whether the firmware was successfully
 *            received.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_uart_done(bool successful);

/**
 * @brief Schedule update of the image.
 *
 * This call does nothing for this target type.
 *
 * @param[in] img_num This parameter is unused by this target type.
 *
 * @return 0, it is always successful.
 **/
int dfu_target_uart_schedule_update(int img_num);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_UART_H__ */
