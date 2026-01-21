/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_MULTI_IMAGE_SAMPLE_COMMON_H__
#define DFU_MULTI_IMAGE_SAMPLE_COMMON_H__

#include <stdint.h>

#define DFU_MULTI_IMAGE_BUFFER_SIZE 1024
extern uint8_t dfu_multi_image_sample_buffer[DFU_MULTI_IMAGE_BUFFER_SIZE];

/**
 * @brief Prepare the dfu_multi_image library for operation with the
 *        sample.
 */
int dfu_multi_image_sample_lib_prepare(void);

#endif /* DFU_MULTI_IMAGE_SAMPLE_COMMON_H__ */
