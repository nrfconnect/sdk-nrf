/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <dfu/pcd.h>
#include <logging/log.h>
#include <storage/stream_flash.h>

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

struct pcd_cmd *pcd_get_cmd(void *addr)
{
	struct pcd_cmd *cmd = (struct pcd_cmd *)addr;

	if (cmd->magic != PCD_CMD_MAGIC_COPY) {
		return NULL;
	}

	return cmd;
}

int pcd_invalidate(struct pcd_cmd *cmd)
{
	if (cmd == NULL) {
		return -EINVAL;
	}

	cmd->magic = PCD_CMD_MAGIC_FAIL;

	return 0;
}

int pcd_transfer(struct pcd_cmd *cmd, struct device *fdev)
{
	struct stream_flash_ctx stream;
	uint8_t buf[CONFIG_PCD_BUF_SIZE];
	int rc;

	if (cmd == NULL) {
		return -EINVAL;
	}

	rc = stream_flash_init(&stream, fdev, buf, sizeof(buf),
			       cmd->offset, 0, NULL);
	if (rc != 0) {
		LOG_ERR("stream_flash_init failed: %d", rc);
		return rc;
	}

	rc = stream_flash_buffered_write(&stream, (uint8_t *)cmd->src_addr,
					 cmd->len, true);
	if (rc != 0) {
		LOG_ERR("stream_flash_buffered_write fail: %d", rc);
		return rc;
	}

	LOG_INF("Transfer done");
	/* Signal complete by setting magic to 0 */
	cmd->magic = PCD_CMD_MAGIC_DONE;
	/* Wait for being rebooted */
	while(1);
	CODE_UNREACHABLE;
	return 0;
}
