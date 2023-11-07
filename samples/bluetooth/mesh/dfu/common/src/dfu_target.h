/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Bluetooth Mesh DFU Target role handler
 */

#ifndef DFU_TARGET_H__
#define DFU_TARGET_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct bt_mesh_dfu_srv dfu_srv;

int dfu_target_init(struct bt_mesh_blob_io_flash *flash_stream);
void dfu_target_image_confirm(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_H__ */
