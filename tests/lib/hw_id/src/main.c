/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/fff.h>

#include <hw_id.h>

#include <bluetooth/bluetooth.h>
#include <zephyr/drivers/hwinfo.h>
#include <modem/modem_jwt.h>
#include <nrf_modem_at.h>
#include <zephyr/net/net_if.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, bt_le_oob_get_local, uint8_t, struct bt_le_oob *);
FAKE_VALUE_FUNC(int, modem_jwt_get_uuids, struct nrf_device_uuid *, struct nrf_modem_fw_uuid *);
FAKE_VALUE_FUNC(struct net_if *, net_if_get_default);
FAKE_VALUE_FUNC(ssize_t, z_impl_hwinfo_get_device_id, uint8_t *, size_t);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd, void *, size_t, const char *, ...);

static uint8_t link_addr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
static struct net_if_dev net_if_dev_example =  {
	.link_addr = {
		.addr = link_addr,
		.len = ARRAY_SIZE(link_addr)
	}
};
static struct net_if net_if_example =  {
	.if_dev = &net_if_dev_example
};

static struct bt_le_oob bt_le_oob_example = {
	.addr.a.val = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}
};
static uint8_t serial_nr[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
static struct nrf_device_uuid uuid_example = {
	.str = "7439023c-5467-11ed-a841-1b85829d924f"
};
static const char imei[] = "985802356545846";

static int custom_bt_le_oob_get_local(uint8_t id, struct bt_le_oob *oob)
{
	TEST_ASSERT_EQUAL(BT_ID_DEFAULT, id);
	*oob = bt_le_oob_example;
	return 0;
}

static ssize_t custom_z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	TEST_ASSERT(length >= ARRAY_SIZE(serial_nr));
	memcpy(buffer, serial_nr, ARRAY_SIZE(serial_nr));
	return ARRAY_SIZE(serial_nr);
}

static int custom_modem_jwt_get_uuids(struct nrf_device_uuid *dev,
			struct nrf_modem_fw_uuid *mfw)
{
	ARG_UNUSED(mfw);
	*dev = uuid_example;
	return 0;
}

static int custom_nrf_modem_at_cmd(void *buf, size_t len, const char *fmt, va_list args)
{
	TEST_ASSERT_GREATER_THAN(ARRAY_SIZE(imei)-1, len);
	TEST_ASSERT_EQUAL(0, strcmp("AT+CGSN", fmt));
	memcpy(buf, imei, ARRAY_SIZE(imei));
	return 0;
}


void setUp(void)
{
	RESET_FAKE(bt_le_oob_get_local);
	RESET_FAKE(modem_jwt_get_uuids);
	RESET_FAKE(net_if_get_default);
	RESET_FAKE(z_impl_hwinfo_get_device_id);
	RESET_FAKE(nrf_modem_at_cmd);
	bt_le_oob_get_local_fake.custom_fake = custom_bt_le_oob_get_local;
	modem_jwt_get_uuids_fake.custom_fake = custom_modem_jwt_get_uuids;
	net_if_get_default_fake.return_val = &net_if_example;
	z_impl_hwinfo_get_device_id_fake.custom_fake = custom_z_impl_hwinfo_get_device_id;
	nrf_modem_at_cmd_fake.custom_fake = custom_nrf_modem_at_cmd;
}

void tearDown(void)
{
}

void test_good_weather(void)
{
	char buf[HW_ID_LEN] = "";
	int err = hw_id_get(buf, ARRAY_SIZE(buf));

	printk("[%s]\n", buf);

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_LESS_THAN(0, strcmp("", buf));
}

void test_null(void)
{
	char buf[HW_ID_LEN] = "";
	int err = hw_id_get(NULL, ARRAY_SIZE(buf));

	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_too_small(void)
{
	char buf[HW_ID_LEN-1] = "";
	int err = hw_id_get(buf, ARRAY_SIZE(buf));

	TEST_ASSERT_EQUAL(-EINVAL, err);
	TEST_ASSERT_EQUAL(0, strcmp("", buf));
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	/* use the runner from test_runner_generate() */
	(void)unity_main();

	return 0;
}
