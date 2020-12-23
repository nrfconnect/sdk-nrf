/* main.c - Application main entry point */

/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <math.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>
#include <drivers/sensor.h>

#if defined(CONFIG_SOC_SERIES_NRF52X)
/* TEMP peripheral is owned by network core in nRF53 series */
#define TEMP_NODE_LABEL DT_LABEL(DT_INST(0, nordic_nrf_temp))
#endif

struct scan_data {
	bt_addr_le_t addr;
	int8_t rssi;
	uint8_t adv_type;
} __aligned(4);

K_MSGQ_DEFINE(scan_data_msgq, sizeof(struct scan_data), 30, 4);

static int8_t get_chip_temp(void)
{
	int err;
	int8_t temperature;

#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* nRF53 needs to talk to network core to get temperature */
	struct net_buf *rsp;

	/* Temperature acquisition time varies between ~700 and ~1500us */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_CHIP_TEMP, NULL, &rsp);
	if (err) {
		printk("HCI Error getting chip temp %d\n", err);
		return -127;
	}

	struct bt_hci_rp_vs_read_chip_temp *cmd_data = (void *)rsp->data;

	temperature = cmd_data->temps;
	net_buf_unref(rsp);

#else
	/* nRF52 series can use normal sensor API */
	const struct device *temp_sensor = device_get_binding(TEMP_NODE_LABEL);
	struct sensor_value sensor_val;

	err = sensor_sample_fetch(temp_sensor);

	if (err == 0) {
		err = sensor_channel_get(temp_sensor, SENSOR_CHAN_DIE_TEMP,
					 &sensor_val);
	}
	if (err != 0) {
		return -127;
	}

	temperature = (int8_t)(sensor_val.val1 + (sensor_val.val2 / 1000000));

#endif

	return temperature;
}

#define ROUND_FL_TO_UINT8(x) (uint8_t)((float)x + 0.5f)

static int8_t compensate_rssi(struct scan_data *p_scan_data)
{
	int8_t temp = get_chip_temp();

#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* See ERRATA 87 (link in README.rst) */
	uint8_t comp_rssi = (uint8_t)((-1) * p_scan_data->rssi);

	/* Compute time is ~20us */
	comp_rssi = ROUND_FL_TO_UINT8(
		+ (4.9e-5f * pow(comp_rssi, 3))
		- (9.9e-3f * pow(comp_rssi, 2))
		+ (1.56f * comp_rssi)
		- (0.05f * temp)
		- 7.2f);

	p_scan_data->rssi = (-1) * (int8_t)comp_rssi;

#endif
#if (defined(CONFIG_SOC_NRF52820_QDAA) || defined(CONFIG_SOC_NRF52833_QIAA) || \
     defined(CONFIG_SOC_NRF52840_QIAA))
	/* See ERRATA 153 and 225 (link in README.rst)*/
	if (temp <= -30) {
		p_scan_data->rssi += 3;
	} else if (temp > -30 && temp <= -10) {
		p_scan_data->rssi += 2;
	} else if (temp > -10 && temp <= 10) {
		p_scan_data->rssi += 1;
	} else if (temp > 30 && temp <= 50) {
		p_scan_data->rssi -= 1;
	} else if (temp > 50 && temp <= 70) {
		p_scan_data->rssi -= 2;
	} else if (temp > 70 && temp <= 85) {
		p_scan_data->rssi -= 3;
	}
#endif
#if (defined(CONFIG_SOC_NRF52820_QDAA) || defined(CONFIG_SOC_NRF52833_QIAA))
	/* only nRF52833/20 are qualified for >85C operation */
	else if (temp > 85) {
		p_scan_data->rssi -= 4;
	}
#endif

	return temp;
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
	struct scan_data scan_data_item;

	memcpy(&scan_data_item.addr, addr, sizeof(bt_addr_le_t));
	scan_data_item.rssi = rssi;
	scan_data_item.adv_type = adv_type;

	/* Silently drop scan data/items if the queue if full */
	k_msgq_put(&scan_data_msgq, &scan_data_item, K_NO_WAIT);
}

void main(void)
{
	int err;
	struct scan_data scan_data_item;

	printk("Starting scanner RSSI sample\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	printk("Starting scan for devices\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_cb);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}

	printk("Waiting for callbacks\n");

	while (1) {
		k_msgq_get(&scan_data_msgq, &scan_data_item, K_FOREVER);

		int8_t temperature = compensate_rssi(&scan_data_item);

		if (scan_data_item.rssi > -70) {
			char addrstr[BT_ADDR_LE_STR_LEN] = { 0 };

			bt_addr_le_to_str(&scan_data_item.addr, addrstr,
					  BT_ADDR_LE_STR_LEN);

			printk("Device: %s, RSSI: %d dBm, Temperature: %d C\n",
			       addrstr, scan_data_item.rssi, temperature);
		}
	}
}
