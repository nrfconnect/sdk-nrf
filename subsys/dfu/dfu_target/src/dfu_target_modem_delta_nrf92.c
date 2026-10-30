/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_modem_delta.h>
#include <zephyr/sys/util.h>

#define MODEM_MAGIC 0x7544656d

struct modem_delta_header {
	uint16_t pad1;
	uint16_t pad2;
	uint32_t magic;
};

bool dfu_target_modem_delta_identify(const void *const buf)
{
	return ((const struct modem_delta_header *)buf)->magic == MODEM_MAGIC;
}

int dfu_target_modem_delta_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(file_size);
	ARG_UNUSED(img_num);
	ARG_UNUSED(cb);

	return -ENOTSUP;
}

int dfu_target_modem_delta_offset_get(size_t *offset)
{
	*offset = 0;
	return 0;
}

int dfu_target_modem_delta_write(const void *const buf, size_t len)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return -ENOTSUP;
}

int dfu_target_modem_delta_done(bool successful)
{
	ARG_UNUSED(successful);

	return -ENOTSUP;
}

int dfu_target_modem_delta_schedule_update(int img_num)
{
	ARG_UNUSED(img_num);

	return -ENOTSUP;
}

int dfu_target_modem_delta_reset(void)
{
	return 0;
}

int dfu_target_modem_delta_firmware_uuid(char *uuid, size_t uuid_len)
{
	if (uuid == NULL || uuid_len < DFU_TARGET_MODEM_DELTA_UUID_LEN) {
		return -EINVAL;
	}

	memset(uuid, 0, uuid_len);
	return -ENOTSUP;
}
