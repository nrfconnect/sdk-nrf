/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>

#define SAMPLE_PERIOD_MS	100

#define UART_BUF_SIZE		64

const static enum sensor_channel sensor_channels[] = {
	SENSOR_CHAN_ACCEL_X,
	SENSOR_CHAN_ACCEL_Y,
	SENSOR_CHAN_ACCEL_Z
};

static const struct device *sensor_dev = DEVICE_DT_GET(DT_NODELABEL(sensor_sim));
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static atomic_t uart_busy;


static void uart_cb(const struct device *dev, struct uart_event *evt,
		    void *user_data)
{
	if (evt->type == UART_TX_DONE) {
		(void)atomic_set(&uart_busy, false);
	}
}

static int init(void)
{
	if (!device_is_ready(sensor_dev)) {
		printk("Sensor device not ready\n");
		return -ENODEV;
	}

	if (!device_is_ready(uart_dev)) {
		printk("UART device not ready\n");
		return -ENODEV;
	}

	int err = uart_callback_set(uart_dev, uart_cb, NULL);

	if (err) {
		printk("Cannot set UART callback (err %d)\n", err);
	}

	return err;
}

static int provide_sensor_data(void)
{
	struct sensor_value data[ARRAY_SIZE(sensor_channels)];
	int err = 0;

	/* Sample simulated sensor. */
	for (size_t i = 0; (!err) && (i < ARRAY_SIZE(sensor_channels)); i++) {
		err = sensor_sample_fetch_chan(sensor_dev, sensor_channels[i]);
		if (!err) {
			err = sensor_channel_get(sensor_dev, sensor_channels[i],
						 &data[i]);
		}
	}

	if (err) {
		printk("Sensor sampling error (err %d)\n", err);
		return err;
	}

	/* Send data over UART. */
	static uint8_t buf[UART_BUF_SIZE];

	if (!atomic_cas(&uart_busy, false, true)) {
		printk("UART not ready\n");
		printk("Please use lower sampling frequency\n");
		return -EBUSY;
	}

	int res = snprintf((char *)buf, sizeof(buf), "%.2f,%.2f,%.2f\r\n",
			   sensor_value_to_double(&data[0]),
			   sensor_value_to_double(&data[1]),
			   sensor_value_to_double(&data[2]));

	if (res < 0) {
		printk("snprintf returned error (err %d)\n", res);
		return res;
	} else if (res >= sizeof(buf)) {
		printk("UART_BUF_SIZE is too small to store the data\n");
		printk("%d bytes are required\n", res);
		return -ENOMEM;
	}

	err = uart_tx(uart_dev, buf, res, SYS_FOREVER_MS);

	if (err) {
		printk("Cannot send data over UART (err %d)\n", err);
	} else {
		printk("Sent data: %s", buf);
	}

	return err;
}

int main(void)
{
	if (init()) {
		return 0;
	}
	printk("Initialized\n");

	int64_t uptime = k_uptime_get();
	int64_t next_timeout = uptime + SAMPLE_PERIOD_MS;

	while (1) {
		if (provide_sensor_data()) {
			break;
		}

		uptime = k_uptime_get();

		if (next_timeout <= uptime) {
			printk("Sampling frequency is too high for sensor\n");
			break;
		}

		k_sleep(K_MSEC(next_timeout - uptime));
		next_timeout += SAMPLE_PERIOD_MS;
	}

	return 0;
}
