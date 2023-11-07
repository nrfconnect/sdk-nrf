/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_MULTI_IMAGE_H__
#define DFU_MULTI_IMAGE_H__

/**
 * @defgroup dfu_multi_image DFU Multi Image
 * @brief Provides an API for writing DFU Multi Image package
 *
 * DFU Multi Image package is a general-purpose update file consisting of a CBOR-based
 * header that describes contents of the package, followed by a number of update
 * components, such as firmware images for different MCU cores. More specifically, the
 * header contains signed numeric identifiers and sizes of included images. The meaning
 * of the identifiers is application-specific, that is, a user is allowed to assign
 * arbitrary identifiers to their images.
 *
 * A DFU Multi Image package can be built manually using either the Python script
 * located at 'scripts/bootloader/dfu_multi_image_tool.py' or the CMake wrapper defined
 * in 'cmake/dfu_multi_image.cmake'. Additionally, @c DFU_MULTI_IMAGE_PACKAGE_BUILD and
 * related Kconfig options are available to enable building of a package that includes
 * common update images.
 *
 * The DFU Multi Image library can be used to process a DFU Multi Image package downloaded
 * onto a device during the DFU process. Its proper usage consists of the following steps:
 *
 * 1. Call @c dfu_multi_image_init function to initialize the library's context.
 * 2. Call @c dfu_multi_image_register_writer for each image identifier you would like to
 *    extract from the package. Images included in the package for which no corresponding
 *    writers have been registered will be ignored.
 * 3. Pass subsequent downloaded chunks of the package to @c dfu_multi_image_write
 *    function. The chunks must be provided in order. Note that if the function returns
 *    an error, no more chunks shall be provided.
 * 4. Call @c dfu_multi_image_done function to release open resources and verify that all
 *    data declared in the header have been written properly.
 *
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*dfu_image_open_t)(int image_id, size_t image_size);
typedef int (*dfu_image_write_t)(const uint8_t *chunk, size_t chunk_size);
typedef int (*dfu_image_close_t)(bool success);

/**
 * @brief User-provided functions for writing a single image from DFU Multi Image package.
 */
struct dfu_image_writer {
	/**
	 * @brief Identifier of the applicable image.
	 */
	int image_id;

	/**
	 * @brief Function called before writing the first byte of the applicable image.
	 *
	 * The function is indirectly called by @c dfu_multi_image_write and in the case
	 * of failure the error code is propagated and returned from the latter function.
	 *
	 * @return negative On failure.
	 * @return 0        On success.
	 */
	dfu_image_open_t open;

	/** @brief Function called to write a subsequent chunk of the applicable image.
	 *
	 * The function is indirectly called by @c dfu_multi_image_write and in the case
	 * of failure the error code is propagated and returned from the latter function.
	 *
	 * @return negative On failure.
	 * @return 0        On success.
	 */
	dfu_image_write_t write;

	/**
	 * @brief Function called after writing the last byte of the applicable image.
	 *
	 * The function is indirectly called by @c dfu_multi_image_write or
	 * @c dfu_multi_image_done and in the case of failure the error code is propagated
	 * and returned from the latter function.
	 *
	 * @return negative On failure.
	 * @return 0        On success.
	 */
	dfu_image_close_t close;
};

/**
 * @brief Initialize DFU Multi Image library context.
 *
 * Resets the internal state of the DFU Multi Image library and initializes necessary
 * resources. In particular, the function removes all writers previously registered with
 * the @c dfu_multi_image_register_writer function.
 *
 * @param[in] buffer Buffer to store DFU Multi Image header in case it spans multiple
 *                   chunks of the package. The buffer is only needed until the header
 *                   is parsed, so the same buffer can be provided to the
 *                   @c dfu_target_mcuboot_set_buf function.
 * @param[in] buffer_size Size of the buffer to store DFU Multi Image header.
 *
 * @return -EINVAL If the provided buffer is too small.
 * @return 0       On success.
 */
int dfu_multi_image_init(uint8_t *buffer, size_t buffer_size);

/**
 * @brief Register DFU image writer.
 *
 * Registers functions for opening, writing and closing a single image included in
 * the downloaded DFU Multi Image package.
 *
 * @param[in] writer Structure that contains the applicable image identifier and
 *                   functions to be registered.
 *
 * @return -ENOMEM If the image writer could not be registered due to lack of empty slots.
 * @return 0	   On success.
 */
int dfu_multi_image_register_writer(const struct dfu_image_writer *writer);

/**
 * @brief Write subsequent DFU Multi Image package chunk.
 *
 * Called to consume a subsequent chunk of the DFU Multi Image package.
 * The initial bytes of the package contain a header that allows the library to learn
 * offsets of particular images within the entire package. After the header is parsed,
 * the registered image writers are used to store the image data.
 *
 * The package chunks must be provided in order.
 *
 * When an image for which no writer has been registered is found in the package, the
 * library may decide to skip ahead the image. For that reason, a user of the function
 * must provide the @c offset argument to validate the chunk's position against the
 * current write position and potentially drop the bytes that are not needed anymore.
 *
 * A user shall NOT write any more chunks after any write results in a failure.
 *
 * @param[in] offset Offset of the chunk within the entire package.
 * @param[in] chunk Pointer to the chunk's data.
 * @param[in] chunk_size Size of the chunk.
 *
 * @retval -ESPIPE  If @c offset is bigger than expected which may indicate a data gap
 *                  or writing more data than declared in the package header.
 * @return negative On other failure.
 * @return 0        On success.
 */
int dfu_multi_image_write(size_t offset, const uint8_t *chunk, size_t chunk_size);

/**
 * @brief Returns DFU Multi Image package write position.
 *
 * @return Offset of the next needed package chunk in bytes.
 */
size_t dfu_multi_image_offset(void);

/**
 * @brief Complete DFU Multi Image package write.
 *
 * Close an open image writer if such exists. Additionally, if @c success argument is
 * true, the function validates that all images listed in the package header have been
 * fully written.
 *
 * @param[in] success Indicates that a user expects all the package contents to have
 *                    been written successfully.
 *
 * @return -ESPIPE  If @c success and not all package contents have been written yet.
 * @return negative On other failure.
 * @return 0        On success.
 */
int dfu_multi_image_done(bool success);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DFU_MULTI_IMAGE_H__ */
