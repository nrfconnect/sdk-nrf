/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file pcd.h
 *
 * @defgroup pcd Peripheral Core DFU
 * @{
 * @brief API for handling DFU of peripheral cores.
 *
 * The PCD provides functions for sending/receiving DFU images between
 * cores on a multi core system.
 */

#ifndef PCD_H__
#define PCD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

/** Magic value written to indicate that a copy should take place. */
#define PCD_CMD_MAGIC_COPY 0xb5b4b3b6
/** Magic value written to indicate that a copy failed. */
#define PCD_CMD_MAGIC_FAIL 0x25bafc15
/** Magic value written to indicate that a copy failed. */
#define PCD_CMD_MAGIC_DONE 0xf103ce5d

/** @brief PCD command structure.
 *
 *  The command is used to communicate information between the sender
 *  and the receiver of the DFU image.
 */
struct pcd_cmd {
	uint32_t magic;   /* Magic value to identify this structure in memory */
	void * src_addr;  /* Source address to copy from */
	size_t len;       /* Number of bytes to copy */
	size_t offset;    /* Offset to store the flash image in */
} __attribute__ ((aligned(4)));

#if DT_HAS_CHOSEN(zephyr_ipc_shm)
#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_BASE_ADDRESS    DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE            sizeof(pcd_cmd);
#endif

#if defined(SHM_BASE_ADDRESS) && (SHM_BASE_ADDRESS != 0)
#define PCD_CMD_ADDRESS SHM_BASE_ADDRESS
#else
#error "Could not find shared memory address"
#endif

/** @brief Get a PCD CMD from the specified address.
 *
 * @param addr The address to check for a valid PCD CMD.
 *
 * @retval A pointer to the PCD CMD if successful.
 *           Otherwise, NULL is returned.
 */
struct pcd_cmd *pcd_get_cmd(void *addr);

/** @brief Update the PCD CMD to invalidate the magic value, indicating that
 * the copy failed.
 *
 * @param cmd The PCD CMD to invalidate.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_invalidate(struct pcd_cmd *cmd);

/** @brief Perform the DFU image transfer.
 *
 * Use the information in the provided PCD CMD to load a DFU image to the
 * provided flash device.
 *
 * @param cmd The PCD CMD whose configuration will be used for the transfer.
 * @param fdev The flash device to transfer the DFU image to.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_transfer(struct pcd_cmd *cmd, struct device *fdev);

#endif /* PCD_H__ */

/**@} */
