/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <device.h>
#include <dfu/pcd.h>
#include <logging/log.h>
#include <storage/stream_flash.h>

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

/** Magic value written to indicate that a copy should take place. */
#define PCD_CMD_MAGIC_COPY 0xb5b4b3b6
/** Magic value written to indicate that a something failed. */
#define PCD_CMD_MAGIC_FAIL 0x25bafc15
/** Magic value written to indicate that a copy is done. */
#define PCD_CMD_MAGIC_DONE 0xf103ce5d

#if defined(CONFIG_SOC_NRF5340_CPUAPP) && defined(CONFIG_MCUBOOT)
#include <pm_config.h>
#include <hal/nrf_reset.h>
#include <hal/nrf_spu.h>
/** Offset which the application should be copied into */
#define NET_CORE_APP_OFFSET PM_CPUNET_B0N_CONTAINER_SIZE
#endif

struct pcd_cmd {
	uint32_t magic; /* Magic value to identify this structure in memory */
	const void *data;     /* Data to copy*/
	size_t len;           /* Number of bytes to copy */
	off_t offset;         /* Offset to store the flash image in */
} __aligned(4);

static struct pcd_cmd *cmd = (struct pcd_cmd *)PCD_CMD_ADDRESS;

void pcd_fw_copy_invalidate(void)
{
	cmd->magic = PCD_CMD_MAGIC_FAIL;
}

enum pcd_status pcd_fw_copy_status_get(void)
{
	if (cmd->magic == PCD_CMD_MAGIC_COPY) {
		return PCD_STATUS_COPY;
	} else if (cmd->magic == PCD_CMD_MAGIC_DONE) {
		return PCD_STATUS_COPY_DONE;
	}

	return PCD_STATUS_COPY_FAILED;
}

const void *pcd_cmd_data_ptr_get(void)
{
	return cmd->data;
}

int pcd_fw_copy(const struct device *fdev)
{
	struct stream_flash_ctx stream;
	uint8_t buf[CONFIG_PCD_BUF_SIZE];
	int rc;

	if (cmd->magic != PCD_CMD_MAGIC_COPY) {
		return -EFAULT;
	}

	rc = stream_flash_init(&stream, fdev, buf, sizeof(buf),
			       cmd->offset, 0, NULL);
	if (rc != 0) {
		LOG_ERR("stream_flash_init failed: %d", rc);
		return rc;
	}

	rc = stream_flash_buffered_write(&stream, (uint8_t *)cmd->data,
					 cmd->len, true);
	if (rc != 0) {
		LOG_ERR("stream_flash_buffered_write fail: %d", rc);
		return rc;
	}

	LOG_INF("Transfer done");

	return 0;
}

void pcd_fw_copy_done(void)
{
	/* Signal complete by setting magic to DONE */
	cmd->magic = PCD_CMD_MAGIC_DONE;
}

#if defined(CONFIG_SOC_NRF5340_CPUAPP) && defined(CONFIG_MCUBOOT)

/** @brief Construct a PCD CMD for copying data/firmware.
 *
 * @param data   The data to copy.
 * @param len    The number of bytes that should be copied.
 * @param offset The offset within the flash device to write the data to.
 *               For internal flash, the offset is the same as the address.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
static int pcd_cmd_write(const void *data, size_t len, off_t offset)
{
	if (data == NULL || len == 0) {
		return -EINVAL;
	}

	cmd->magic = PCD_CMD_MAGIC_COPY;
	cmd->data = data;
	cmd->len = len;
	cmd->offset = offset;

	return 0;
}

int pcd_network_core_update(const void *src_addr, size_t len)
{
	int err;

	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 * This is needed for the network core to be able to read the
	 * shared RAM area used for IPC.
	 */
	nrf_spu_extdomain_set(NRF_SPU, 0, true, false);

	/* Ensure that the network core is turned off */
	nrf_reset_network_force_off(NRF_RESET, true);

	err = pcd_cmd_write(src_addr, len, NET_CORE_APP_OFFSET);
	if (err != 0) {
		LOG_INF("Error while writing PCD cmd: %d", err);
		return err;
	}

	nrf_reset_network_force_off(NRF_RESET, false);
	LOG_INF("Turned on network core");

	do {
		/* Wait for 1 second to avoid issue where network core
		 * is unable to write to shared RAM.
		 */
		k_busy_wait(1 * USEC_PER_SEC);

		err = pcd_fw_copy_status_get();
	} while (err == PCD_STATUS_COPY);

	if (err == PCD_STATUS_COPY_FAILED) {
		LOG_ERR("Network core update failed");
		return err;
	}

	nrf_reset_network_force_off(NRF_RESET, true);
	LOG_INF("Turned off network core");

	return 0;
}

void pcd_lock_ram(void)
{
	uint32_t region = PCD_CMD_ADDRESS/CONFIG_NRF_SPU_RAM_REGION_SIZE;

	nrf_spu_ramregion_set(NRF_SPU, region, true, NRF_SPU_MEM_PERM_READ,
			true);
}
#endif /* CONFIG_SOC_NRF5340_CPUAPP && CONFIG_MCUBOOT */
