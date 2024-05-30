/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <bluetooth/enocean.h>

#define FLAG_ACTIVE BIT(0)
#define FLAG_DIRTY BIT(1)

/** Mocks ******************************************/

static struct bt_enocean_device mock_enocean_sensor = {
	.addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {
			.val = {0x11, 0x22, 0x45, 0x67, 0x00, 0xE5}
		}
	}
};

static struct bt_enocean_device mock_enocean_switch = {
	.addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {
			.val = {0x11, 0x22, 0x45, 0x67, 0x15, 0xE2}
		}
	}
};

static struct bt_enocean_device exp_enocean_dev;
struct device_entry {
	bt_addr_le_t addr;
	uint8_t key[16];
};

static struct device_entry exp_dev_entry;
static uint8_t exp_energy_lvl = 0x68/2;
static uint16_t exp_light_sensor = 0x018D;


/* Mock bt_le_scan_cb_register to capture the callback from enocean.c so that
 * we can use it for testing the module.
 */
static struct bt_le_scan_cb *scancb;
void bt_le_scan_cb_register(struct bt_le_scan_cb *cb)
{
	scancb = cb;
}

int settings_save_one(const char *name, const void *value, size_t val_len)
{
	ztest_check_expected_data(name, 14);
	ztest_check_expected_value(val_len);
	ztest_check_expected_data(value, val_len);
	return ztest_get_return_value();
}

int settings_delete(const char *name)
{
	return 0;
}

int bt_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13],
		const uint8_t *enc_data, size_t len, const uint8_t *aad,
		size_t aad_len, uint8_t *plaintext, size_t mic_size)
{
	/* This test does not check decrypt function call */
	return 0;
}


/** End of mocks ***********************************/

static void enocean_button_cb(struct bt_enocean_device *device,
		       enum bt_enocean_button_action action, uint8_t changed,
		       const uint8_t *opt_data, size_t opt_data_len)
{
	ztest_check_expected_data(device, sizeof(struct bt_enocean_device));
	ztest_check_expected_value(action);
	ztest_check_expected_value(changed);
	ztest_check_expected_value(opt_data_len);
	/* This test does not use "optional data" */
}

static void enocean_sensor_cb(struct bt_enocean_device *device,
		       const struct bt_enocean_sensor_data *data,
		       const uint8_t *opt_data, size_t opt_data_len)
{
	ztest_check_expected_data(device, sizeof(struct bt_enocean_device));
	zassert_true(*data->energy_lvl == exp_energy_lvl, "Invalid energy level value");
	zassert_true(*data->light_sensor == exp_light_sensor, "Invalid light sensor value");
	ztest_check_expected_value(opt_data_len);
	/* This test does not use "optional data" */
}

static void enocean_commissioned_cb(struct bt_enocean_device *device)
{
	ztest_check_expected_data(device, sizeof(struct bt_enocean_device));
}

static void enocean_decommissioned_cb(struct bt_enocean_device *device)
{
	/* */
}

static void enocean_loaded_cb(struct bt_enocean_device *device)
{
	/* */
}

static const struct bt_enocean_callbacks cbs = {
	.button = enocean_button_cb,
	.sensor = enocean_sensor_cb,
	.commissioned = enocean_commissioned_cb,
	.decommissioned = enocean_decommissioned_cb,
	.loaded = enocean_loaded_cb
};

static void adv_packet_create(struct net_buf_simple *buf, uint8_t ad_len,
			uint8_t *ad_data)
{
	net_buf_simple_reset(buf);

	/* Total PDU len + ADType*/
	net_buf_simple_add_u8(buf, ad_len + 1);
	/* ADType (0xFF) */
	net_buf_simple_add_u8(buf, 0xFF);
	/* Add Data */
	net_buf_simple_add_mem(buf, ad_data, ad_len);
}

static void send_adv(const struct bt_enocean_device *dev, struct net_buf_simple *buf)
{
	static struct bt_le_scan_recv_info scan_info = {
		.sid = 1,
		.rssi = 0,
		.tx_power = 0,
		.adv_type = BT_GAP_ADV_TYPE_ADV_NONCONN_IND,
	};

	scan_info.addr = &dev->addr,

	scancb->recv(&scan_info, buf);
}

#define ADV_DATA_SIZE_MAX 31

static int name_i;

static void comm_expect(uint32_t seq, uint8_t flags, uint8_t *key,
			uint8_t *addr_val,
			struct bt_enocean_device *exp_enocean_dev,
			uint8_t lexp_dev_entry,
			struct device_entry *exp_dev_entry)
{
	static char name[20] = {0};

	exp_enocean_dev->addr.type = BT_ADDR_LE_RANDOM;
	memcpy(exp_enocean_dev->addr.a.val, addr_val, 6);
	exp_enocean_dev->seq = seq;
	exp_enocean_dev->flags = flags;
	exp_enocean_dev->rssi = 0;
	memcpy(exp_enocean_dev->key, key, 16);
	memcpy(exp_dev_entry->key, key, 16);
	memcpy(&exp_dev_entry->addr, &exp_enocean_dev->addr, sizeof(bt_addr_le_t));

	exp_enocean_dev->addr.type = BT_ADDR_LE_RANDOM;
	ztest_expect_data(enocean_commissioned_cb, device, exp_enocean_dev);
	sprintf(name, "bt/enocean/%d/d", name_i++);
	ztest_expect_data(settings_save_one, name, name);
	ztest_expect_value(settings_save_one, val_len, lexp_dev_entry);
	ztest_expect_data(settings_save_one, value, exp_dev_entry);
	ztest_returns_value(settings_save_one, 0);
}

static void sensor_data_expect(uint32_t seq, uint8_t flags,
			struct bt_enocean_device *exp_enocean_dev)
{
	exp_enocean_dev->seq = seq;
	exp_enocean_dev->flags = flags;
	ztest_expect_data(enocean_sensor_cb, device, exp_enocean_dev);
	ztest_expect_value(enocean_sensor_cb, opt_data_len, 0);
}

static void switch_data_expect(uint32_t seq, uint8_t flags, uint8_t action_val,
		uint8_t changed_val, struct bt_enocean_device *exp_enocean_dev)
{
	exp_enocean_dev->seq = seq;
	exp_enocean_dev->flags = flags;
	ztest_expect_value(enocean_button_cb, action, action_val);
	ztest_expect_value(enocean_button_cb, changed, changed_val);
	ztest_expect_data(enocean_button_cb, device, exp_enocean_dev);
	ztest_expect_value(enocean_button_cb, opt_data_len, 0);
}

#define SW_KEYOFF_FROM_MFGID (6)
#define SEN_KEYOFF_FROM_MFGID (7)
#define SW_STATUSOFF_FROM_MFGID (6)

ZTEST(enocean_ts, test_messages)
{
	NET_BUF_SIMPLE_DEFINE(buf, ADV_DATA_SIZE_MAX);

	/* Test sample data - Sensor */
	/* Data telegram example - STM550B DS*/
	/* Note: Sequence number of the packet is changed to 0x2DFB to prevent
	 * Sequence number check from failing (since DS unfortunately does not
	 * take into account this issue while specifying sample data), therefore
	 * the PDU won't authenticate, and this unit test does not check crypto
	 * anyway.
	 */
	uint8_t data_sensor[] = {0xDA, 0x03, 0xFB, 0x2D, 0x00, 0x00, 0x02, 0x68, 0x45,
		0x8D, 0x01, 0x40, 0x42, 0x09, 0x06, 0x57, 0x8A, 0xF7, 0x91, 0xE6,
		0x5E, 0x23, 0x01, 0x0F, 0x01, 0x57, 0xD3};

	/* Data telegram example - STM550B DS*/
	uint8_t comm_sensor[] = {0xDA, 0x03, 0xE4, 0x2D, 0x00, 0x00, 0x3E, 0x4D,
		0xB3, 0x4D, 0xB3, 0x07, 0x0E, 0xFC, 0x67, 0x13, 0xFE, 0x39, 0xE1,
		0x3C, 0xF3, 0xC9, 0x39, 0x00, 0x00, 0x01, 0x77, 0x00, 0xE5};

	comm_expect(0x2DE4, FLAG_ACTIVE, &comm_sensor[SEN_KEYOFF_FROM_MFGID],
			mock_enocean_sensor.addr.a.val, &exp_enocean_dev,
			sizeof(exp_dev_entry), &exp_dev_entry);
	adv_packet_create(&buf, sizeof(comm_sensor), comm_sensor);
	send_adv(&mock_enocean_sensor, &buf);


	/* Test: Data telegram example */
	/* expect settings_save_one call to be triggered and verify param values */
	sensor_data_expect(0x2DFB, FLAG_ACTIVE | FLAG_DIRTY, &exp_enocean_dev);
	adv_packet_create(&buf, sizeof(data_sensor), data_sensor);
	send_adv(&mock_enocean_sensor, &buf);

	/* Test sample data - Switch */
	/* Data captured from real switch */
	uint8_t comm_sw[] = {0xda, 0x03, 0x38, 0x0d, 0x00, 0x00, 0x2c,
		0x4f, 0x3d, 0xe4, 0x05, 0x14, 0x74, 0x1e, 0xc3, 0x54, 0x7c, 0x07,
		0xbc, 0xd7, 0x54, 0xda, 0xee, 0xb6, 0x00, 0x00, 0x15, 0xe2};

	uint8_t data_sw[] = {0xda, 0x03, 0x3b, 0x0d, 0x00, 0x00, 0x08,
		0x6f, 0x43, 0xc5, 0xcc};

	/* Test: commissioning telegram */
	comm_expect(0x0d38, FLAG_ACTIVE, &comm_sw[SW_KEYOFF_FROM_MFGID],
			mock_enocean_switch.addr.a.val, &exp_enocean_dev,
			sizeof(exp_dev_entry), &exp_dev_entry);
	adv_packet_create(&buf, sizeof(comm_sw), comm_sw);
	send_adv(&mock_enocean_switch, &buf);

	/* Test: data telegram */
	switch_data_expect(0x0d3b, FLAG_ACTIVE | FLAG_DIRTY,
				data_sw[SW_STATUSOFF_FROM_MFGID] & BIT(0),
				data_sw[SW_STATUSOFF_FROM_MFGID] >> 1,
				&exp_enocean_dev);
	adv_packet_create(&buf, sizeof(data_sw), data_sw);
	send_adv(&mock_enocean_switch, &buf);
}

static void setup(void *f)
{
	bt_enocean_init(&cbs);
	bt_enocean_commissioning_enable();
}

ZTEST_SUITE(enocean_ts, NULL, NULL, setup, NULL, NULL);
