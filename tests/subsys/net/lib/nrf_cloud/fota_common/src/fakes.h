/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, fota_download_s0_active_get, bool *const);
FAKE_VALUE_FUNC(bool, boot_is_img_confirmed);
FAKE_VALUE_FUNC(int, boot_write_img_confirmed);
FAKE_VALUE_FUNC(int, dfu_target_full_modem_cfg, const struct dfu_target_full_modem_params *);
FAKE_VALUE_FUNC(int, dfu_target_full_modem_fdev_get, struct dfu_target_fmfu_fdev * const);

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
FAKE_VALUE_FUNC(int, nrf_modem_lib_init);
FAKE_VALUE_FUNC(int, nrf_modem_lib_shutdown);
FAKE_VALUE_FUNC(int, nrf_modem_lib_bootloader_init);
FAKE_VALUE_FUNC(int, fmfu_fdev_load, uint8_t *, size_t, const struct device *, size_t);
#endif /* CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE */

int fake_fota_download_s0_active_get__fails(bool *const s0)
{
	ARG_UNUSED(s0);
	return -1;
}

int fake_fota_download_s0_active_get__s0_active(bool *const s0)
{
	*s0 = true;
	return 0;
}
int fake_fota_download_s0_active_get__s0_inactive(bool *const s0)
{
	*s0 = false;
	return 0;
}

bool fake_boot_is_img_confirmed__true(void)
{
	return true;
}

bool fake_boot_is_img_confirmed__false(void)
{
	return false;
}

int fake_boot_write_img_confirmed__succeeds(void)
{
	return 0;
}

int fake_boot_write_img_confirmed__fails(void)
{
	return -EIO;
}

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
int fake_dfu_target_full_modem_cfg__succeeds(const struct dfu_target_full_modem_params *p)
{
	ARG_UNUSED(p);
	return 0;
}

int fake_dfu_target_full_modem_cfg__fails(const struct dfu_target_full_modem_params *p)
{
	ARG_UNUSED(p);
	return -1;
}

int fake_dfu_target_full_modem_fdev_get__succeeds(struct dfu_target_fmfu_fdev * const p)
{
	ARG_UNUSED(p);
	return 0;
}

int fake_dfu_target_full_modem_fdev_get__fails(struct dfu_target_fmfu_fdev * const p)
{
	ARG_UNUSED(p);
	return -1;
}

int nrf_modem_lib_shutdown__succeeds(void)
{
	return 0;
}

int nrf_modem_lib_shutdown__fails(void)
{
	return -1;
}

int nrf_modem_lib_bootloader_init__succeeds(void)
{
	return 0;
}

int nrf_modem_lib_bootloader_init__fails(void)
{
	return -1;
}

int fmfu_fdev_load__succeeds(uint8_t *buf, size_t buf_len,
	const struct device *fdev, size_t offset)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	ARG_UNUSED(fdev);
	ARG_UNUSED(offset);
	return 0;
}

int fmfu_fdev_load__fails(uint8_t *buf, size_t buf_len,
	const struct device *fdev, size_t offset)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	ARG_UNUSED(fdev);
	ARG_UNUSED(offset);
	return -1;
}
#endif /* CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE */
