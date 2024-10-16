/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <modem/modem_jwt.h>

DEFINE_FFF_GLOBALS;

#if !IS_ENABLED(CONFIG_NRF_MODEM_LIB)
FAKE_VALUE_FUNC(int, nrf_modem_lib_init);
FAKE_VALUE_FUNC(int, nrf_modem_lib_shutdown);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd, void *, size_t, const char *, ...);
FAKE_VALUE_FUNC(int, modem_jwt_get_uuids, struct nrf_device_uuid *, struct nrf_modem_fw_uuid *);
FAKE_VALUE_FUNC(int, hw_id_get, char *, size_t);

int nrf_modem_lib_shutdown__succeeds(void)
{
	return 0;
}

int nrf_modem_lib_shutdown__fails(void)
{
	return -1;
}

int nrf_modem_at_cmd__fails(void *buf, size_t len, const char *fmt, ...)
{
	return -1;
}

int nrf_modem_at_cmd__succeeds(void *buf, size_t len, const char *fmt, ...)
{
	return 0;
}

int modem_jwt_get_uuids__fails(struct nrf_device_uuid *dev,
			struct nrf_modem_fw_uuid *mfw)
{
	return -1;
}

int modem_jwt_get_uuids__succeeds(struct nrf_device_uuid *dev,
			struct nrf_modem_fw_uuid *mfw)
{
	return 0;
}

int hw_id_get__fails(char *buf, size_t buf_len)
{
	return -1;
}

int hw_id_get__succeeds(char *buf, size_t buf_len)
{
	return 0;
}
#endif
