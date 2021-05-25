/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifndef DFU_HOST_H__
#define DFU_HOST_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**@brief Set up a DFU procedure */
int dfu_host_setup(void);

/**@brief Start to send init packet */
int dfu_host_send_ip(const uint8_t *data, uint32_t data_size);

/**@brief Start to send firmware bin */
int dfu_host_send_fw(const uint8_t *data, uint32_t data_size);

/**@brief Check if it's in bootloader mode */
bool dfu_host_bl_mode_check(void);

#ifdef __cplusplus
}   /* ... extern "C" */
#endif  /* __cplusplus */


#endif /* _INC_DFU_SERIAL */
