/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file pcd.h
 *
 * @defgroup pcd Peripheral Core DFU
 * @{
 * @brief API for handling DFU of peripheral cores.
 *
 * The PCD API provides functions for transferring DFU images from a generic
 * core to a peripheral core to which there is no flash access from the generic
 * core.
 *
 * The cores communicate through a command structure (CMD) which is stored in
 * a shared memory location.
 *
 * The nRF5340 is an example of a system with these properties.
 */

#ifndef PCD_H__
#define PCD_H__

#include <device.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_SERIES_NRF53X

#include <pm_config.h>

#ifdef PM_PCD_SRAM_ADDRESS
#define PCD_CMD_ADDRESS PM_PCD_SRAM_ADDRESS
#else
/* extra '_' since its in a different domain */
#define PCD_CMD_ADDRESS PM__PCD_SRAM_ADDRESS
#endif /* PM_PCD_SRAM_ADDRESS */

#endif

enum pcd_status {
	PCD_STATUS_COPY = 0,
	PCD_STATUS_COPY_DONE = 1,
	PCD_STATUS_COPY_FAILED = 2
};

/** @brief Sets up the PCD command structure with the location and size of the
 *	   firmware update. Then boots the network core and checks if the
 *	   update completed successfully.
 *
 * @param src_addr Start address of the data which is to be copied into the
 *                 network core.
 * @param len Length of the data which is to be copied into the network core.
 *
 * @retval 0 on success, -1 on failure.
 */
int pcd_network_core_update(const void *src_addr, size_t len);

/** @brief Lock the RAM section used for IPC with the network core bootloader.
 */
void pcd_lock_ram(void);

/** @brief Update the PCD CMD to indicate that the copy operation has completed
 *         successfully.
 */
void pcd_fw_copy_done(void);

/** @brief Invalidate the PCD CMD, indicating that the copy failed.
 */
void pcd_fw_copy_invalidate(void);

/** @brief Check the PCD CMD to find the status of the update.
 *
 * @return PCD_STATUS_COPY if update is pending
 * @return PCD_STATUS_COPY_DONE if update has completed successfully
 * @return PCD_STATUS_COPY_FAILED if update has completed unsuccessfully
 */
enum pcd_status pcd_fw_copy_status_get(void);

/** @brief Get value of 'data' member of pcd cmd.
 *
 * @retval value of 'data' member.
 */
const void *pcd_cmd_data_ptr_get(void);

/** @brief Perform the DFU image transfer.
 *
 * Use the information in the PCD CMD to load a DFU image to the
 * provided flash device.
 *
 * @param fdev The flash device to transfer the DFU image to.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_fw_copy(const struct device *fdev);

#endif /* PCD_H__ */

/**@} */
