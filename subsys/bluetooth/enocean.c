/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/enocean.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/crypto.h>

#define LOG_LEVEL CONFIG_BT_ENOCEAN_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_enocean);

#include "common/bt_str.h"

#define SIGNATURE_LEN 4
#define SETTINGS_TAG_SIZE 17

#define DATA_TYPE_COMMISSIONING 0x3e
#define DATA_TYPE_LIGHT_LEVEL_SENSOR 0x05
#define DATA_TYPE_OCCUPANCY 0x20
#define DATA_TYPE_BATTERY 0x01
#define DATA_TYPE_ENERGY_LEVEL 0x02
#define DATA_TYPE_LIGHT_LEVEL_SOLAR_CELL 0x04
#define DATA_TYPE_OPTIONAL_DATA 0x3c

#define DEVICE_TYPE_SWITCH 0xe215
#define DEVICE_TYPE_SENSOR 0xe500

#define FLAG_ACTIVE BIT(0)
#define FLAG_DIRTY BIT(1)

struct __packed nonce {
	uint8_t addr[6];
	uint32_t seq;
	uint8_t padding[3];
};

struct device_entry {
	bt_addr_le_t addr;
	uint8_t key[16];
};

enum entry_tag {
	ENTRY_TAG_DEVICE = 'd',
	ENTRY_TAG_SEQ = 's',
};

static struct bt_enocean_device devices[CONFIG_BT_ENOCEAN_DEVICES_MAX];
static const struct bt_enocean_callbacks *cb;
static struct k_work_delayable work;
static bool commissioning;

static struct bt_enocean_device *device_find(const bt_addr_le_t *addr)
{
	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		if ((devices[i].flags & FLAG_ACTIVE) &&
		    !bt_addr_le_cmp(addr, &devices[i].addr)) {
			return &devices[i];
		}
	}

	LOG_DBG("Unknown device %s", bt_addr_le_str(addr));
	return NULL;
}

static struct bt_enocean_device *device_alloc(const bt_addr_le_t *addr,
					      uint32_t seq, const uint8_t key[16])
{
	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		if (!(devices[i].flags & FLAG_ACTIVE)) {
			bt_addr_le_copy(&devices[i].addr, addr);
			devices[i].seq = seq;
			memcpy(devices[i].key, key, sizeof(devices[i].key));
			devices[i].flags = 0;
			return &devices[i];
		}
	}

	return NULL;
}

static void encode_tag(char buf[SETTINGS_TAG_SIZE], uint8_t index,
		       enum entry_tag tag)
{
	snprintk(buf, SETTINGS_TAG_SIZE, "bt/enocean/%u/%c", index, tag);
}

static void schedule_store(void)
{
#if CONFIG_BT_ENOCEAN_STORE_SEQ
	/* Only start timer if it's not already running */
	k_work_schedule(&work, K_SECONDS(CONFIG_BT_ENOCEAN_STORE_TIMEOUT));
#endif
}

static int store_new_dev(const struct bt_enocean_device *dev)
{
	if (!IS_ENABLED(CONFIG_BT_ENOCEAN_STORE)) {
		return 0;
	}

	int index = dev - &devices[0];
	struct device_entry entry;
	char tag[SETTINGS_TAG_SIZE];
	int err;

	bt_addr_le_copy(&entry.addr, &dev->addr);
	memcpy(entry.key, dev->key, sizeof(entry.key));

	encode_tag(tag, index, ENTRY_TAG_DEVICE);
	err = settings_save_one(tag, &entry, sizeof(entry));
	if (err) {
		return err;
	}

	if (!IS_ENABLED(CONFIG_BT_ENOCEAN_STORE_SEQ)) {
		return 0;
	}

	encode_tag(tag, index, ENTRY_TAG_SEQ);
	return settings_save_one(tag, &dev->seq, 4);
}

static int auth(const struct bt_enocean_device *dev, uint32_t seq,
		const uint8_t *signature, const uint8_t *payload, uint8_t len)
{
	struct nonce nonce;
	int err;

	memcpy(nonce.addr, dev->addr.a.val, sizeof(nonce.addr));
	nonce.seq = seq;
	memset(nonce.padding, 0, sizeof(nonce.padding));

	err = bt_ccm_decrypt(dev->key, (uint8_t *)&nonce, signature, 0, payload,
			     len, NULL, SIGNATURE_LEN);
	if (err) {
		return err;
	}

	return 0;
}

static void handle_switch_commissioning(const struct bt_le_scan_recv_info *info,
					struct net_buf_simple *buf,
					bool has_short_name)
{
	if (!commissioning) {
		return;
	}

	uint32_t seq = net_buf_simple_pull_le32(buf);
	const uint8_t *key = net_buf_simple_pull_mem(buf, 16);

	if (!has_short_name && buf->len != 6) {
		return;
	}

	(void)bt_enocean_commission(info->addr, key, seq);
}

static void handle_switch_data(const struct bt_le_scan_recv_info *info,
			       struct net_buf_simple *buf, const uint8_t *payload)
{
	if (!cb->button) {
		return;
	}

	struct bt_enocean_device *dev = device_find(info->addr);
	int err;

	if (!dev) {
		return;
	}

	uint32_t seq = net_buf_simple_pull_le32(buf);

	if (seq <= dev->seq) {
		return;
	}

	uint8_t status = net_buf_simple_pull_u8(buf);

	/* Top 3 bits must be 0 */
	if (status & BIT_MASK(3) << 5) {
		return;
	}

	size_t opt_data_len = buf->len - SIGNATURE_LEN;
	uint8_t *opt_data = NULL;

	if (opt_data_len != 0 && opt_data_len != 1 && opt_data_len != 2 &&
	    opt_data_len != 4) {
		return;
	}

	if (opt_data_len != 0) {
		opt_data = net_buf_simple_pull_mem(buf, opt_data_len);
	}

	LOG_DBG("%p %u %s: 0b%u%u%u%u", dev, seq,
	       status & BIT(0) ? "pressed" : "released", !!(status & BIT(1)),
	       !!(status & BIT(2)), !!(status & BIT(3)), !!(status & BIT(4)));

	uint8_t *signature = net_buf_simple_pull_mem(buf, SIGNATURE_LEN);

	err = auth(dev, seq, signature, payload, 9 + opt_data_len);
	if (err) {
		LOG_ERR("Auth failed: %d", err);
		return;
	}

	dev->seq = seq;
	dev->rssi = info->rssi;
	dev->flags |= FLAG_DIRTY;
	schedule_store();

	enum bt_enocean_button_action action = status & BIT(0);

	cb->button(dev, action, status >> 1, opt_data, opt_data_len);
}

static void handle_switch_payload(const struct bt_le_scan_recv_info *info,
				  struct net_buf_simple *buf, const uint8_t *payload, uint8_t len,
				  bool has_shortened_name)
{
	/* The data format is decided by a mix of lengths and bitfields */
	if (len == 29 || (has_shortened_name && len == 23)) {
		handle_switch_commissioning(info, buf, has_shortened_name);
		return;
	}

	if (!has_shortened_name && !(payload[8] & (BIT_MASK(3) << 5)) && len >= 12 && len <= 16) {
		handle_switch_data(info, buf, payload);
		return;
	}

	LOG_DBG("Unknown switch payload");
}

struct data_entry {
	uint8_t type;
	uint8_t len;
	union {
		uint32_t value;
		const uint8_t *pointer;
	};
};

static int data_entry_pull(struct net_buf_simple *buf,
			    struct data_entry *entry)
{
	uint8_t header = net_buf_simple_pull_u8(buf);

	entry->type = header & BIT_MASK(6);

	if ((header >> 6) == 0x03 || entry->type == DATA_TYPE_OPTIONAL_DATA) {
		entry->len = net_buf_simple_pull_u8(buf);
		if (buf->len < entry->len) {
			LOG_WRN("Malformed data");
			return -EINVAL;
		}
		entry->pointer = net_buf_simple_pull_mem(buf, entry->len);
		return 0;
	}

	/* Commissioning is special */
	if (entry->type == DATA_TYPE_COMMISSIONING) {
		if (buf->len < 22) {
			LOG_WRN("Malformed data");
			return -EINVAL;
		}
		entry->len = 22;
		entry->pointer = net_buf_simple_pull_mem(buf, 22);
		return 0;
	}

	entry->len = 1 << (header >> 6);
	if (entry->len == 8 || buf->len < entry->len) {
		LOG_WRN("Malformed data");
		return -EINVAL;
	}

	entry->value = 0;
	memcpy(&entry->value, net_buf_simple_pull_mem(buf, entry->len), entry->len);
	return 0;
}

static void handle_sensor_data(const struct bt_le_scan_recv_info *info,
			       struct net_buf_simple *buf, const uint8_t *payload,
			       uint8_t tot_len)
{
	if (!cb->sensor) {
		return;
	}

	struct bt_enocean_device *dev;
	struct data_entry entry;
	uint32_t seq;
	int err;

	seq = net_buf_simple_pull_le32(buf);

	dev = device_find(info->addr);
	if (!dev) {
		err = data_entry_pull(buf, &entry);
		if (entry.type != DATA_TYPE_COMMISSIONING || !commissioning || err) {
			return;
		}

		bt_enocean_commission(info->addr, entry.pointer, seq);
		return;
	}

	if (seq <= dev->seq) {
		return;
	}

	struct bt_enocean_sensor_data data = { 0 };
	const uint8_t *opt_data = NULL;
	uint8_t opt_data_len = 0;
	bool occupancy;
	uint16_t light_sensor;
	uint16_t battery_voltage;
	uint16_t light_solar_cell;
	uint8_t energy_lvl;

	LOG_DBG("Sensor data:");
	while (buf->len >= 2 + SIGNATURE_LEN) {
		err = data_entry_pull(buf, &entry);
		if (err) {
			return;
		}

		switch (entry.type) {
		case DATA_TYPE_BATTERY:
			battery_voltage = entry.value / 2;
			data.battery_voltage = &battery_voltage;
			LOG_DBG("\tBattery:        %u mV", battery_voltage);
			break;
		case DATA_TYPE_ENERGY_LEVEL:
			energy_lvl = entry.value / 2;
			data.energy_lvl = &energy_lvl;
			LOG_DBG("\tEnergy level:   %u %%", energy_lvl);
			break;
		case DATA_TYPE_LIGHT_LEVEL_SENSOR:
			light_sensor = entry.value;
			data.light_sensor = &light_sensor;
			LOG_DBG("\tLight (sensor): %u lx", light_sensor);
			break;
		case DATA_TYPE_LIGHT_LEVEL_SOLAR_CELL:
			light_solar_cell = entry.value;
			data.light_solar_cell = &light_solar_cell;
			LOG_DBG("\tLight (solar):  %u lx", light_solar_cell);
			break;
		case DATA_TYPE_OCCUPANCY:
			occupancy = entry.value == 2;
			data.occupancy = &occupancy;
			LOG_DBG("\tOccupancy:      %s",
			       occupancy ? "true" : "false");
			break;
		case DATA_TYPE_OPTIONAL_DATA:
			opt_data = entry.pointer;
			opt_data_len = entry.len;
			LOG_DBG("\tOptional data (%u bytes)", opt_data_len);
			break;
		case DATA_TYPE_COMMISSIONING:
			LOG_DBG("\tCommissioning");
			return;
		default:
			LOG_DBG("\tUnknown type:   %x", entry.type);
			break;
		}
	}

	if (buf->len != SIGNATURE_LEN) {
		return;
	}

	const uint8_t *signature = net_buf_simple_pull_mem(buf, SIGNATURE_LEN);

	err = auth(dev, seq, signature, payload, tot_len);
	if (err) {
		LOG_ERR("Auth failed: %d", err);
		return;
	}

	dev->seq = seq;
	dev->rssi = info->rssi;
	dev->flags |= FLAG_DIRTY;
	schedule_store();

	cb->sensor(dev, &data, opt_data, opt_data_len);
}

static void adv_recv(const struct bt_le_scan_recv_info *info,
		     struct net_buf_simple *buf)
{
	if (info->adv_type != BT_GAP_ADV_TYPE_ADV_NONCONN_IND || !info->addr ||
	    info->addr->type != BT_ADDR_LE_RANDOM || (buf->len < 2)) {
		return;
	}

	uint8_t *payload = buf->data;
	uint8_t len = net_buf_simple_pull_u8(buf);
	uint8_t type = net_buf_simple_pull_u8(buf);

	if (len < 1 || buf->len < (len - 1)) {
		LOG_WRN("Malformed data");
		return;
	}

	bool has_shortened_name = (type == BT_DATA_NAME_SHORTENED);
	/* Upper two bytes of address indicate device type */
	uint16_t device_type = sys_get_le16(&info->addr->a.val[4]);

	/* An old version of EnOcean firmware would include the shortened name
	 * AD type, skip this.
	 */
	if (has_shortened_name) {

		if (buf->len < ((len - 1) + 2) || device_type != DEVICE_TYPE_SWITCH) {
			return;
		}

		net_buf_simple_pull(buf, len - 1);
		payload = buf->data;
		len = net_buf_simple_pull_u8(buf);
		type = net_buf_simple_pull_u8(buf);
	}

	if ((type != BT_DATA_MANUFACTURER_DATA) || (len == 0) ||
	    (buf->len < 2) || (buf->len < len - 1)) {
		return;
	}

	uint16_t manufacturer = net_buf_simple_pull_le16(buf);

	if (manufacturer != 0x03da) {
		return;
	}

	switch (device_type) {
	case DEVICE_TYPE_SENSOR:
		handle_sensor_data(info, buf, payload, len + 1 - SIGNATURE_LEN);
		break;
	case DEVICE_TYPE_SWITCH:
		handle_switch_payload(info, buf, payload, len, has_shortened_name);
		break;
	default:
		break;
	}
}

static void store_dirty(struct k_work *work)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		if (!(devices[i].flags & FLAG_DIRTY)) {
			continue;
		}

		char tag[SETTINGS_TAG_SIZE];

		encode_tag(tag, i, ENTRY_TAG_SEQ);
		err = settings_save_one(tag, &devices[i].seq,
					sizeof(devices[i].seq));
		if (err) {
			LOG_WRN("#%u err: %d", i, err);
			break;
		}

		LOG_DBG("Stored #%u", i);

		devices[i].flags &= ~FLAG_DIRTY;
	}

	if (err) {
		schedule_store();
	}
}

static int settings_set(const char *key, size_t len, settings_read_cb read_cb,
			void *cb_arg)
{
	struct bt_enocean_device *dev;
	struct device_entry entry;
	const char *tag;
	int size;

	uint32_t index = atoi(key);

	if (index >= ARRAY_SIZE(devices)) {
		return -ENOMEM;
	}

	dev = &devices[index];

	(void)settings_name_next(key, &tag);

	if (tag[0] == ENTRY_TAG_DEVICE) {
		size = read_cb(cb_arg, &entry, sizeof(entry));
		if (size < sizeof(entry)) {
			return -EINVAL;
		}

		bt_addr_le_copy(&dev->addr, &entry.addr);
		memcpy(dev->key, entry.key, sizeof(dev->key));
		dev->flags |= FLAG_ACTIVE;

		LOG_DBG("Loaded %s", bt_addr_le_str(&dev->addr));
		return 0;
	}

	if (tag[0] == ENTRY_TAG_SEQ) {
		size = read_cb(cb_arg, &dev->seq, sizeof(dev->seq));
		if (size < sizeof(dev->seq)) {
			return -EINVAL;
		}

		return 0;
	}

	return -EINVAL;
}

static int process_loaded_devs(void)
{
	if (!cb->loaded) {
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		if (!(devices[i].flags & FLAG_ACTIVE)) {
			continue;
		}

		cb->loaded(&devices[i]);
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_enocean, "bt/enocean", NULL, settings_set,
			       process_loaded_devs, NULL);

void bt_enocean_init(const struct bt_enocean_callbacks *callbacks)
{
	if (IS_ENABLED(CONFIG_BT_ENOCEAN_STORE_SEQ)) {
		k_work_init_delayable(&work, store_dirty);
	}

	cb = callbacks;

	process_loaded_devs();

	static struct bt_le_scan_cb scan_cb = { .recv = adv_recv };

	bt_le_scan_cb_register(&scan_cb);
}

void bt_enocean_commissioning_enable(void)
{
	commissioning = true;
}

void bt_enocean_commissioning_disable(void)
{
	commissioning = false;
}

int bt_enocean_commission(const bt_addr_le_t *addr, const uint8_t key[16],
			  uint32_t seq)
{
	struct bt_enocean_device *dev;
	int err;

	dev = device_find(addr);
	if (dev) {
		return -EEXIST;
	}

	dev = device_alloc(addr, seq, key);
	if (!dev) {
		LOG_WRN("No more slots");
		return -ENOMEM;
	}

	dev->flags |= FLAG_ACTIVE;

	if (cb->commissioned) {
		cb->commissioned(dev);

		/* The user might reject the device inside the callback: */
		if (!(dev->flags & FLAG_ACTIVE)) {
			return -ECANCELED;
		}
	}

	err = store_new_dev(dev);
	if (err) {
		LOG_WRN("Storage failed: %d", err);
		return -EBUSY;
	}

	return 0;
}

void bt_enocean_decommission(struct bt_enocean_device *dev)
{
	char name[SETTINGS_TAG_SIZE];

	if (IS_ENABLED(CONFIG_BT_ENOCEAN_STORE)) {
		encode_tag(name, dev - &devices[0], ENTRY_TAG_DEVICE);
		settings_delete(name);
	}

	if (IS_ENABLED(CONFIG_BT_ENOCEAN_STORE_SEQ)) {
		encode_tag(name, dev - &devices[0], ENTRY_TAG_SEQ);
		settings_delete(name);
	}

	dev->flags = 0;

	if (cb->decommissioned) {
		cb->decommissioned(dev);
	}
}

uint32_t bt_enocean_foreach(bt_enocean_foreach_cb_t cb, void *user_data)
{
	uint32_t count = 0;

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		if (devices[i].flags & FLAG_ACTIVE) {
			count++;
			cb(&devices[i], user_data);
		}
	}

	return count;
}
