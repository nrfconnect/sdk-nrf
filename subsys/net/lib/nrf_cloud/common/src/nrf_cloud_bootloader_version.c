/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fw_info_bare.h>
#include <nrf_cloud_bootloader_version.h>
#include <tfm/tfm_ioctl_api.h>

BUILD_ASSERT(DT_REG_SIZE(DT_NODELABEL(s0_partition)) != 0,
	     "s0_partition must have non-zero size");
BUILD_ASSERT(DT_REG_SIZE(DT_NODELABEL(s1_partition)) != 0,
	     "s1_partition must have non-zero size");

#define NRF_CLOUD_B1_S0_ADDRESS DT_REG_ADDR(DT_NODELABEL(s0_partition))
#define NRF_CLOUD_B1_S1_ADDRESS DT_REG_ADDR(DT_NODELABEL(s1_partition))

BUILD_ASSERT(NRF_CLOUD_B1_S0_ADDRESS != NRF_CLOUD_B1_S1_ADDRESS,
	     "MCUboot B1 slot addresses must differ");

LOG_MODULE_REGISTER(nrf_cloud_bootloader_version, CONFIG_NRF_CLOUD_LOG_LEVEL);

static int active_b1_fw_info_get(struct fw_info *info)
{
	bool s0_active;
	int err;

	err = tfm_platform_s0_active(NRF_CLOUD_B1_S0_ADDRESS, NRF_CLOUD_B1_S1_ADDRESS,
				     &s0_active);
	if (err) {
		return err;
	}

	return tfm_platform_firmware_info(
		s0_active ? NRF_CLOUD_B1_S0_ADDRESS : NRF_CLOUD_B1_S1_ADDRESS, info);
}

int nrf_cloud_bootloader_version_string_get(char *buf, size_t len)
{
	int err;
	int written;
	struct fw_info info;

	if ((buf == NULL) || (len == 0U)) {
		LOG_ERR("Invalid buffer");
		return -EINVAL;
	}

	err = active_b1_fw_info_get(&info);
	if (err) {
		LOG_ERR("active_b1_fw_info_get, error: %d", err);
		return err;
	}

	if (info.valid != CONFIG_FW_INFO_VALID_VAL) {
		LOG_ERR("Invalid firmware info");
		return -ENOENT;
	}

	written = snprintk(buf, len, "%u", info.version);
	if ((written < 0) || (written >= len)) {
		LOG_ERR("Failed to format bootloader version");
		return -ENOMEM;
	}

	return 0;
}
