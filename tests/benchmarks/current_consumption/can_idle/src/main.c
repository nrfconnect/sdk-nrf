/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

#define CAN_TEST_STD_ID	      0x123U
#define CAN_TEST_BITRATE      250000
#define CAN_TEST_SAMPLE_POINT 875
#define CAN_TEST_DLC	      8

#define CAN_SEND_TIMEOUT K_MSEC(100)
#define CAN_RECV_TIMEOUT K_MSEC(100)

#define CAN_CLASSIC_FRAME_BITS(dlc_bytes) (47U + (dlc_bytes) * 8U + ((47U + (dlc_bytes) * 8U) / 4U))

#define CAN_ACTIVE_ITERATIONS                                                                      \
	((CAN_TEST_BITRATE * 1000) / (CAN_CLASSIC_FRAME_BITS(CAN_TEST_DLC) * 1000U))

CAN_MSGQ_DEFINE(can_msgq, 4);

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct can_frame test_frame(void)
{
	struct can_frame frame = {
		.flags = 0U,
		.id = CAN_TEST_STD_ID,
		.dlc = CAN_TEST_DLC,
		.data = {1, 2, 3, 4, 5, 6, 7, 8},
	};

	return frame;
}

static int can_idle_setup(void)
{
	struct can_timing timing = {0};
	struct can_filter filter = {
		.flags = 0U,
		.id = CAN_TEST_STD_ID,
		.mask = CAN_STD_ID_MASK,
	};
	int err;

	__ASSERT(device_is_ready(can_dev), "CAN device not ready");

	k_object_access_grant(can_dev, k_current_get());
	k_object_access_grant(&can_msgq, k_current_get());

	err = can_stop(can_dev);
	if (err != 0 && err != -EALREADY) {
		return err;
	}

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	if (err != 0) {
		return err;
	}

	err = can_calc_timing(can_dev, &timing, CAN_TEST_BITRATE, CAN_TEST_SAMPLE_POINT);
	if (err < 0) {
		return err;
	}

	err = can_set_timing(can_dev, &timing);
	if (err != 0) {
		return err;
	}

	err = can_add_rx_filter_msgq(can_dev, &can_msgq, &filter);
	if (err < 0) {
		return err;
	}

	return 0;
}

int main(void)
{
	struct can_frame frame = test_frame();
	struct can_frame rx_frame;
	int err;

	err = gpio_is_ready_dt(&led);
	__ASSERT(err == 1, "Error: LED gpio is not ready");

	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(err == 0, "Could not configure led GPIO");

	err = can_idle_setup();
	__ASSERT(err == 0, "CAN setup failed (%d)", err);

	while (1) {
		err = can_start(can_dev);
		__ASSERT(err == 0, "can_start failed (%d)", err);

		for (uint32_t i = 0; i < CAN_ACTIVE_ITERATIONS; i++) {
			err = can_send(can_dev, &frame, CAN_SEND_TIMEOUT, NULL, NULL);
			__ASSERT(err == 0, "can_send failed (%d)", err);

			err = k_msgq_get(&can_msgq, &rx_frame, CAN_RECV_TIMEOUT);
			__ASSERT(err == 0, "RX msgq timeout (%d)", err);
			__ASSERT(rx_frame.id == frame.id, "unexpected frame ID");
		}

		err = can_stop(can_dev);
		__ASSERT(err == 0 || err == -EALREADY, "can_stop failed (%d)", err);

		gpio_pin_set_dt(&led, 0);
		k_msleep(1000);
		gpio_pin_set_dt(&led, 1);
	}

	return 0;
}
