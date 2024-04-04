/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <dfu/pcd.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PCD_NET
#ifdef CONFIG_PCD_READ_NETCORE_APP_VERSION
#include <fw_info_bare.h>
#endif
#include <zephyr/storage/stream_flash.h>
#endif

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

/** Magic value written to indicate that a copy should take place. */
#define PCD_CMD_MAGIC_COPY 0xb5b4b3b6
/** Magic value written to indicate that a something failed. */
#define PCD_CMD_MAGIC_FAIL 0x25bafc15
/** Magic value written to indicate that a copy is done. */
#define PCD_CMD_MAGIC_DONE 0xf103ce5d
/** Magic value written to indicate that a version number read should take place. */
#define PCD_CMD_MAGIC_READ_VERSION 0xdca345ea

#ifdef CONFIG_PCD_APP

#include <hal/nrf_reset.h>
#include <hal/nrf_spu.h>

/** Offset which the application should be copied into */
#ifdef CONFIG_PCD_NET_CORE_APP_OFFSET
#define PCD_NET_CORE_APP_OFFSET CONFIG_PCD_NET_CORE_APP_OFFSET
#else
#include <pm_config.h>
#define PCD_NET_CORE_APP_OFFSET PM_CPUNET_B0N_CONTAINER_SIZE
#endif

#define NETWORK_CORE_UPDATE_CHECK_TIME K_SECONDS(1)

static void network_core_finished_check_handler(struct k_timer *timer);

K_TIMER_DEFINE(network_core_finished_check_timer,
	       network_core_finished_check_handler, NULL);

#endif /* CONFIG_PCD_APP */

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
	} else if (cmd->magic == PCD_CMD_MAGIC_READ_VERSION) {
		return PCD_STATUS_READ_VERSION;
	} else if (cmd->magic == PCD_CMD_MAGIC_DONE) {
		return PCD_STATUS_DONE;
	}

	return PCD_STATUS_FAILED;
}

const void *pcd_cmd_data_ptr_get(void)
{
	return cmd->data;
}

#ifdef CONFIG_PCD_NET
#ifdef CONFIG_PCD_READ_NETCORE_APP_VERSION
int pcd_find_fw_version(void)
{
	const struct fw_info *firmware_info;

	if (cmd->magic != PCD_CMD_MAGIC_READ_VERSION) {
		return -EFAULT;
	}

	firmware_info = fw_info_find(PM_APP_ADDRESS);

	if (firmware_info != NULL) {
		memcpy((void *)cmd->data, &firmware_info->version, cmd->len);
		cmd->len = sizeof(firmware_info->version);
		return 0;
	}

	return -EFAULT;
}
#endif

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

void pcd_done(void)
{
	/* Signal complete by setting magic to DONE */
	cmd->magic = PCD_CMD_MAGIC_DONE;
}

#endif /* CONFIG_PCD_NET */

#ifdef CONFIG_PCD_APP

static void network_core_pcd_tidy(void)
{
	/* Configure nRF5340 Network MCU back into Non-Secure domain.
	 * This is the default for the network core when it's reset.
	 */
	nrf_spu_extdomain_set(NRF_SPU, 0, false, false);
}

static void network_core_finished_check_handler(struct k_timer *timer)
{
	if (pcd_fw_copy_status_get() != PCD_STATUS_COPY) {
		/* Upgrade has finished and either failed or completed
		 * successfully, tidy up and cancel timer
		 */
		k_timer_stop(&network_core_finished_check_timer);
		network_core_pcd_tidy();
	}
}

/** @brief Construct a PCD CMD for copying data/firmware.
 *
 * @param data   The data to copy.
 * @param len    The number of bytes that should be copied.
 * @param offset The offset within the flash device to write the data to.
 *               For internal flash, the offset is the same as the address.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
static int pcd_cmd_write(uint32_t command, const void *data, size_t len, off_t offset)
{
	if (data == NULL || len == 0) {
		return -EINVAL;
	}

	cmd->magic = command;
	cmd->data = data;
	cmd->len = len;
	cmd->offset = offset;

	return 0;
}

static int network_core_pcd_cmdset(uint32_t cmd, const void *src_addr, size_t len, bool wait)
{
	int err;
	enum pcd_status command_status;

	/* Ensure that the network core is turned off */
	nrf_reset_network_force_off(NRF_RESET, true);

	err = pcd_cmd_write(cmd, src_addr, len, PCD_NET_CORE_APP_OFFSET);
	if (err != 0) {
		LOG_INF("Error while writing PCD cmd: %d", err);
		return err;
	}

	enum pcd_status initial_command_status = pcd_fw_copy_status_get();

	nrf_reset_network_force_off(NRF_RESET, false);
	LOG_INF("Turned on network core");

	if (!wait) {
		return 0;
	}

	do {
		/* Wait for 1 second to avoid issue where network core
		 * is unable to write to shared RAM.
		 */
		k_busy_wait(1 * USEC_PER_SEC);

		command_status = pcd_fw_copy_status_get();
	} while (command_status == initial_command_status);

	if (command_status == PCD_STATUS_FAILED) {
		LOG_ERR("Network core update failed");
		network_core_pcd_tidy();
		return -EFAULT;
	}

	nrf_reset_network_force_off(NRF_RESET, true);
	LOG_INF("Turned off network core");
	network_core_pcd_tidy();
	return 0;
}

#ifdef CONFIG_PCD_READ_NETCORE_APP_VERSION
int pcd_network_core_app_version(uint8_t *buf, size_t len)
{
	if (buf == NULL || len < 4) {
		return -EINVAL;
	}

	/* Configure nRF5340 Network MCU into Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 * This is needed for the network core to be able to read the
	 * shared RAM area used for IPC.
	 */

	nrf_spu_extdomain_set(NRF_SPU, 0, true, false);

	return network_core_pcd_cmdset(PCD_CMD_MAGIC_READ_VERSION, buf, len, true);
}
#endif

static int network_core_update(const void *src_addr, size_t len, bool wait)
{
	/* Configure nRF5340 Network MCU into Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 * This is needed for the network core to be able to read the
	 * shared RAM area used for IPC.
	 */
	nrf_spu_extdomain_set(NRF_SPU, 0, true, false);

	return network_core_pcd_cmdset(PCD_CMD_MAGIC_COPY, src_addr, len, wait);
}

int pcd_network_core_update_initiate(const void *src_addr, size_t len)
{
	int rc = network_core_update(src_addr, len, false);

	if (rc == 0) {
		k_timer_start(&network_core_finished_check_timer,
			      NETWORK_CORE_UPDATE_CHECK_TIME,
			      NETWORK_CORE_UPDATE_CHECK_TIME);
		k_busy_wait(1 * USEC_PER_SEC);
	}

	return rc;
}

int pcd_network_core_update(const void *src_addr, size_t len)
{
	return network_core_update(src_addr, len, true);
}

void pcd_lock_ram(void)
{
	uint32_t region = PCD_CMD_ADDRESS/CONFIG_NRF_SPU_RAM_REGION_SIZE;

	nrf_spu_ramregion_set(NRF_SPU, region, false, NRF_SPU_MEM_PERM_READ,
			true);
}

#endif /* CONFIG_PCD_APP */
