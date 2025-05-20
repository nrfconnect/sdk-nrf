/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_DIST_H__
#define DFU_DIST_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct bt_mesh_dfd_srv dfd_srv;

void dfu_distributor_init(struct bt_mesh_blob_io_flash *flash_stream);

#ifdef __cplusplus
}
#endif

#endif /* DFU_DIST_H__ */
