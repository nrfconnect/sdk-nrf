/*
 * Copyright (c) 2024 Junho Lee <tot0roprog@gmail.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Blob Transfer
 */

#ifndef BLOB_XFER_H__
#define BLOB_XFER_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TARGET_NODES CONFIG_BT_MESH_BLOB_TX_MAX_TARGETS

#define STORAGE_START 0x0000
#define STORAGE_END 0x8000
#define STORAGE_ALIGN 0x1000

/* FLASH APIs */
typedef void (*blob_cb)(uint8_t, off_t, size_t);
void write_flash_area(uint8_t id, off_t offset, size_t size);
void read_flash_area(uint8_t id, off_t offset, size_t size);
void hash_flash_area(uint8_t id, off_t offset, size_t size);
uint8_t get_flash_area_id(void);

/* BLOB sender/receiver APIs */
int blob_tx_flash_init(off_t offset, size_t size);
int blob_tx_flash_is_init(void);
void blob_tx_targets_init(int targets[], int target_count);
int blob_tx_start(uint64_t blob_id, uint16_t base_timeout, uint16_t group, uint16_t appkey_index,
                  uint8_t ttl, enum bt_mesh_blob_xfer_mode mode);
int blob_tx_cancel(void);
int blob_tx_suspend(void);
int blob_tx_resume(void);
int blob_rx_flash_init(off_t offset);
int blob_rx_flash_is_init(void);
int blob_rx_start(uint64_t blob_id, uint16_t base_timeout, uint8_t ttl);
int blob_rx_cancel(void);
int blob_rx_progress(void);

/* BLOB Transfer Client/Server models initialization */
void blob_xfer_srv_init(struct bt_mesh_blob_srv *srv);
void blob_xfer_cli_init(struct bt_mesh_blob_cli *cli);

#ifdef __cplusplus
}
#endif

#endif /* BLOB_XFER_H__ */
