/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(peripheral_esl);

int display_init(void)
{
	LOG_WRN("No display device init");

	return 0;
}

int display_control(uint8_t disp_idx, uint8_t img_idx, bool enable)
{
	ARG_UNUSED(disp_idx);
	ARG_UNUSED(img_idx);
	ARG_UNUSED(enable);
	LOG_WRN("No display device control");
}

void display_unassociated(uint8_t disp_idx)
{
	ARG_UNUSED(disp_idx);
	LOG_WRN("No display device QR code");
}

void display_associated(uint8_t disp_idx)
{
	ARG_UNUSED(disp_idx);
	LOG_WRN("No display device QR code");
}
