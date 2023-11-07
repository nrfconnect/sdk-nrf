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

#include <zephyr/device.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_SERIES_NRF53X

#ifdef CONFIG_PCD_CMD_ADDRESS

#define PCD_CMD_ADDRESS CONFIG_PCD_CMD_ADDRESS

#else

#include <pm_config.h>

#ifdef PM_PCD_SRAM_ADDRESS
#define PCD_CMD_ADDRESS PM_PCD_SRAM_ADDRESS
#else
/* extra '_' since its in a different domain */
#define PCD_CMD_ADDRESS PM__PCD_SRAM_ADDRESS
#endif /* PM_PCD_SRAM_ADDRESS */

#endif /* CONFIG_PCD_CMD_ADDRESS */

#endif /* CONFIG_SOC_SERIES_NRF53X */

enum pcd_status {
	PCD_STATUS_COPY = 0,
	PCD_STATUS_DONE = 1,
	PCD_STATUS_FAILED = 2,
	PCD_STATUS_READ_VERSION = 3,
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

/** @brief Sets up the PCD command structure with the location and size of the
 *	   firmware update. Then boots the network core and initiates the
 *	   update procedure. The function will return before the update
 *	   procedure on the network core has completed or failed, and will
 *	   indicate success as long as the initialization of the update
 *	   procedure was successful. The network core will still be in a
 *	   powered on state when this function returns.
 *
 * @param src_addr Start address of the data which is to be copied into the
 *                 network core.
 * @param len Length of the data which is to be copied into the network core.
 *
 * @retval 0 on success, an error code on failure.
 */
int pcd_network_core_update_initiate(const void *src_addr, size_t len);

/** @brief Lock the RAM section used for IPC with the network core bootloader.
 */
void pcd_lock_ram(void);

/** @brief Update the PCD CMD to indicate that the operation has completed
 *         successfully.
 */
void pcd_done(void);

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

#ifdef CONFIG_PCD_READ_NETCORE_APP_VERSION
/** @brief Set up the PCD command structure and point the data buffer to version
 *
 * @param buf Pointer to a at least 4 byte sized buffer in RAM.
 * @param len Length of the buffer.
 *
 * @retval 0 on success, -EFAULT and -EINVAL on failure.
 */
int pcd_network_core_app_version(uint8_t *buf, size_t len);

/** @brief Read network core image version.
 *
 * Finds and sets the version number in the data buffer provided by PCD CMD structure.
 *
 * @retval non-negative integer on success, negative on failure.
 */
int pcd_find_fw_version(void);
#endif /* CONFIG_PCD_READ_NETCORE_APP_VERSION */

#ifdef __cplusplus
}
#endif

#endif /* PCD_H__ */

/**@} */
