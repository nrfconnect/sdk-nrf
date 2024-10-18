/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @brief Send a CAN test frame with asserts.
 *
 * This function will block until the frame is queued or a test timeout
 * occurs.
 *
 * @param dev      Pointer to the device structure for the driver instance.
 * @param frame    Pointer to the CAN frame to send.
 * @param callback Transmit callback function.
 */
static void send_test_frame(const struct device *dev, const struct can_frame *frame,
				   can_tx_callback_t callback)
{
	int err;

	err = can_send(dev, frame, TEST_SEND_TIMEOUT, callback, (void *)frame);
	zassert_equal(err, 0, "failed to send frame (err %d)", err);
}

/**
 * @brief Standard (11-bit) CAN ID receive callback.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_std_callback(const struct device *dev, struct can_frame *frame,
			      void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_1, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/* Additional tests */

/**
 * @brief Callback 1 invoked when CAN is stopped during transmission.
 * Standard CAN frame.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_std_callback_netdown_1(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	if ((error == 0) || (error == -ENETDOWN)) {
		/* Frame may be sent before CAN is stopped */
		k_sem_give(&tx_callback_sem);
	}

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_1, "ID does not match");
	TC_PRINT("error1: %d\n", error);
}

/**
 * @brief Callback 2 invoked when CAN is stopped during transmission.
 * Standard CAN frame.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_std_callback_netdown_2(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	if ((error == 0) || (error == -ENETDOWN)) {
		/* Frame may be sent before CAN is stopped */
		k_sem_give(&tx_callback_sem);
	}

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_2, "ID does not match");
	TC_PRINT("error2: %d\n", error);
}

/**
 * @brief Callback 1 invoked when CAN is stopped during transmission.
 * Extended CAN frame.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_ext_callback_netdown_1(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	if ((error == 0) || (error == -ENETDOWN)) {
		/* Frame may be sent before CAN is stopped */
		k_sem_give(&tx_callback_sem);
	}

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_EXT_ID_1, "ID does not match");
	TC_PRINT("error1: %d\n", error);
}

/**
 * @brief Callback 2 invoked when CAN is stopped during transmission.
 * Extended CAN frame.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_ext_callback_netdown_2(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	if ((error == 0) || (error == -ENETDOWN)) {
		/* Frame may be sent before CAN is stopped */
		k_sem_give(&tx_callback_sem);
	}

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_EXT_ID_2, "ID does not match");
	TC_PRINT("error2: %d\n", error);
}

/**
 * @brief Test TX callback is executed when CAN gets stopped.
 * Standard CAN frame.
 */
ZTEST(can_classic, test_tx_callback_when_can_stops_std)
{
	int err;

	k_sem_reset(&tx_callback_sem);

	send_test_frame(can_dev, &test_std_frame_1, tx_std_callback_netdown_1);
	send_test_frame(can_dev, &test_std_frame_2, tx_std_callback_netdown_2);

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = k_sem_take(&tx_callback_sem, K_MSEC(200));
	zassert_equal(err, 0, "missing TX callback");

	err = k_sem_take(&tx_callback_sem, K_MSEC(200));
	zassert_equal(err, 0, "missing TX callback");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test TX callback is executed when CAN gets stopped.
 * Extended CAN frame.
 */
ZTEST(can_classic, test_tx_callback_when_can_stops_ext)
{
	int err;

	k_sem_reset(&tx_callback_sem);

	send_test_frame(can_dev, &test_ext_frame_1, tx_ext_callback_netdown_1);
	send_test_frame(can_dev, &test_ext_frame_2, tx_ext_callback_netdown_2);

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = k_sem_take(&tx_callback_sem, K_MSEC(200));
	zassert_equal(err, 0, "missing TX callback");

	err = k_sem_take(&tx_callback_sem, K_MSEC(200));
	zassert_equal(err, 0, "missing TX callback");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test error when sending invalid frame (too big data payload).
 *
 * This basic test work since the CAN controller is in loopback mode and
 * therefore ACKs its own frame.
 */
ZTEST_USER(can_classic, test_send_invalid_frame)
{
	struct can_frame test_invalid_frame = {
		.flags   = 0,
		.id      = TEST_CAN_STD_ID_1,
		.dlc     = 9,
		.data    = {0}
	};
	int err;

	err = can_send(can_dev, (const struct can_frame *)&test_invalid_frame, TEST_SEND_TIMEOUT,
		       NULL, NULL);
	zassert_not_equal(err, -EBUSY, "arbitration lost in loopback mode");
	zassert_equal(err, -EINVAL, "send incorrect frame (err %d)", err);

	test_invalid_frame.flags = CAN_FRAME_IDE;
	test_invalid_frame.dlc = 65;

	err = can_send(can_dev, (const struct can_frame *) &test_invalid_frame,	TEST_SEND_TIMEOUT,
				NULL, NULL);
	zassert_not_equal(err, -EBUSY, "arbitration lost in loopback mode");
	zassert_equal(err, -EINVAL, "send incorrect frame (err %d)", err);

	test_invalid_frame.flags = CAN_FRAME_ESI;
	test_invalid_frame.dlc = 4;

	err = can_send(can_dev, (const struct can_frame *) &test_invalid_frame,	TEST_SEND_TIMEOUT,
				NULL, NULL);
	zassert_not_equal(err, -EBUSY, "arbitration lost in loopback mode");
	zassert_equal(err, -ENOTSUP, "send incorrect frame (err %d)", err);
}

/**
 * @brief Test setting a too low bitrate.
 */
ZTEST_USER(can_classic, test_set_bitrate_too_low)
{
	uint32_t min = can_get_bitrate_min(can_dev);
	int err;

	if (min == 0) {
		ztest_test_skip();
	}

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_bitrate(can_dev, min - 1);
	zassert_equal(err, -ENOTSUP, "too low bitrate accepted");

	err = can_set_bitrate(can_dev, CONFIG_CAN_DEFAULT_BITRATE);
	zassert_equal(err, 0, "failed to restore default bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test setting bitrate.
 */
ZTEST_USER(can_classic, test_set_bitrate_sweep)
{
	uint32_t test_val[] = {400000, 500000, 640000, 800000, 1000000};
	size_t total = ARRAY_SIZE(test_val);
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	for (size_t i = 0; i < total; i++) {
		err = can_set_bitrate(can_dev, test_val[i]);
		zassert_equal(err, 0, "Can't set bitrate %d (err %d)", test_val[i], err);
	}

	err = can_set_bitrate(can_dev, 550000);
	zassert_equal(err, -ENOTSUP, "Set bitrate 550000 shall fail (err %d)", err);

	err = can_set_bitrate(can_dev, CONFIG_CAN_DEFAULT_BITRATE);
	zassert_equal(err, 0, "failed to restore default bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test setting CAN bitrate to min.
 */
ZTEST_USER(can_classic, test_set_bitrate_min)
{
	uint32_t min = can_get_bitrate_min(can_dev);
	int err;

	TC_PRINT("min: %d\n", min);

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_bitrate(can_dev, min);
	zassert_equal(err, 0, "lowest bitrate NOT accepted");

	err = can_set_bitrate(can_dev, CONFIG_CAN_DEFAULT_BITRATE);
	zassert_equal(err, 0, "failed to restore default bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/*
 * Function can_get_bitrate_max(can_dev) returns highest possible CAN bitrate
 * that is valid only for CAN FD
 */

/**
 * @brief Test using an invalid bitrate.
 */
ZTEST_USER(can_classic, test_invalid_bitrate)
{
	struct can_timing timing;
	uint32_t min = can_get_bitrate_min(can_dev);
	uint32_t max = can_get_bitrate_max(can_dev);
	int err;

	if (min > 0) {
		err = can_calc_timing(can_dev, &timing, 0, TEST_SAMPLE_POINT);
		zassert_equal(err, -EINVAL, "invalid bitrate of 0 bit/s accepted (err %d)", err);
	}

	err = can_calc_timing(can_dev, &timing, max + 1, TEST_SAMPLE_POINT);
	zassert_equal(err, -EINVAL, "invalid bitrate of 1000001 bit/s accepted (err %d)", err);
}

/**
 * @brief Test that timing values for the data phase that are above the maximum can't be set.
 */
ZTEST_USER(can_classic, test_set_timing_above_max)
{
	int err;

	const struct can_timing *max_timing = can_get_timing_max(can_dev);
	struct can_timing test_timing;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	test_timing = *max_timing;
	test_timing.sjw += 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid sjw was accepted (err %d)", err);

	test_timing = *max_timing;
	test_timing.prop_seg += 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid prop_seg was accepted (err %d)", err);

	test_timing = *max_timing;
	test_timing.phase_seg1 += 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid phase_seg1 was accepted (err %d)", err);

	test_timing = *max_timing;
	test_timing.phase_seg2 += 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid phase_seg2 was accepted (err %d)", err);

	test_timing = *max_timing;
	test_timing.prescaler += 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid prescaler was accepted (err %d)", err);

	err = can_set_bitrate(can_dev, CONFIG_CAN_DEFAULT_BITRATE);
	zassert_equal(err, 0, "failed to restore default bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test that timing values for the data phase that are below the minimum can't be set.
 */
ZTEST_USER(can_classic, test_set_timing_below_min)
{
	int err;

	const struct can_timing *min_timing = can_get_timing_min(can_dev);
	struct can_timing test_timing;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	if (min_timing->sjw > 0) {
		test_timing = *min_timing;
		test_timing.sjw -= 1;
		err = can_set_timing(can_dev, &test_timing);
		zassert_equal(err, -ENOTSUP, "invalid sjw was accepted (err %d)", err);
	}

	test_timing = *min_timing;
	test_timing.sjw = min_timing->phase_seg1 + 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid sjw was accepted (err %d)", err);

	test_timing = *min_timing;
	test_timing.sjw = min_timing->phase_seg2 + 1;
	err = can_set_timing(can_dev, &test_timing);
	zassert_equal(err, -ENOTSUP, "invalid sjw was accepted (err %d)", err);

	if (min_timing->prop_seg > 0) {
		test_timing = *min_timing;
		test_timing.prop_seg -= 1;
		err = can_set_timing(can_dev, &test_timing);
		zassert_equal(err, -ENOTSUP, "invalid prop_seg was accepted (err %d)", err);
	}

	if (min_timing->phase_seg1 > 0) {
		test_timing = *min_timing;
		test_timing.phase_seg1 -= 1;
		err = can_set_timing(can_dev, &test_timing);
		zassert_equal(err, -ENOTSUP, "invalid phase_seg1 was accepted (err %d)", err);
	}

	if (min_timing->phase_seg2 > 0) {
		test_timing = *min_timing;
		test_timing.phase_seg2 -= 1;
		err = can_set_timing(can_dev, &test_timing);
		zassert_equal(err, -ENOTSUP, "invalid phase_seg2 was accepted (err %d)", err);
	}

	if (min_timing->prescaler > 0) {
		test_timing = *min_timing;
		test_timing.prescaler -= 1;
		err = can_set_timing(can_dev, &test_timing);
		zassert_equal(err, -ENOTSUP, "invalid prescaler was accepted (err %d)", err);
	}

	err = can_set_bitrate(can_dev, CONFIG_CAN_DEFAULT_BITRATE);
	zassert_equal(err, 0, "failed to restore default bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test adding filter with invalid flags.
 */
ZTEST(canfd, test_add_invalid_filter)
{
	int err;
	const struct can_filter invalid_filter = {
		.flags = 0xFFU,
		.id = TEST_CAN_STD_ID_1,
		.mask = CAN_STD_ID_MASK
	};

	/* Filter flags are invalid */
	err = can_add_rx_filter(can_dev, rx_std_callback, NULL, &invalid_filter);
	zassert_equal(err, -ENOTSUP, "added invalid filter (err %d)", err);
}

/**
 * @brief Test adding filter with NULL arguments.
 */
ZTEST(can_classic, test_add_filter_with_NULLs)
{
	int err;

	/* Callback is NULL */
	err = can_add_rx_filter(can_dev, NULL, NULL, &test_std_filter_1);
	zassert_equal(err, -EINVAL, "added filter with NULL callback");

	/* Filter is NULL */
	err = can_add_rx_filter(can_dev, rx_std_callback, NULL, NULL);
	zassert_equal(err, -EINVAL, "added invalid filter (err %d)", err);
}

/**
 * @brief Test that different CAN modes can be set.
 */
ZTEST_USER(can_classic, test_can_modes)
{
	enum can_state state;
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_STOPPED, "CAN controller not stopped");

	err = can_set_mode(can_dev, CAN_MODE_NORMAL);
	zassert_equal(err, 0, "failed to set normal mode (err %d)", err);
	zassert_equal(CAN_MODE_NORMAL, can_get_mode(can_dev));

	err = can_set_mode(can_dev, CAN_MODE_LISTENONLY);
	if (err == -ENOTSUP) {
		TC_PRINT("CAN_MODE_LISTENONLY is not supported\n");
	} else {
		zassert_equal(err, 0, "failed to set listen-only mode (err %d)", err);
		zassert_equal(CAN_MODE_LISTENONLY, can_get_mode(can_dev));
	}

	err = can_set_mode(can_dev, CAN_MODE_ONE_SHOT);
	if (err == -ENOTSUP) {
		TC_PRINT("CAN_MODE_ONE_SHOT is not supported\n");
	} else {
		zassert_equal(err, 0, "failed to set one-shot mode (err %d)", err);
		zassert_equal(CAN_MODE_ONE_SHOT, can_get_mode(can_dev));
	}

	err = can_set_mode(can_dev, CAN_MODE_3_SAMPLES);
	if (err == -ENOTSUP) {
		TC_PRINT("CAN_MODE_3_SAMPLES is not supported\n");
	} else {
		zassert_equal(err, 0, "failed to set 3-samples mode (err %d)", err);
		zassert_equal(CAN_MODE_3_SAMPLES, can_get_mode(can_dev));
	}

	err = can_set_mode(can_dev, CAN_MODE_MANUAL_RECOVERY);
	if (err == -ENOTSUP) {
		TC_PRINT("CAN_MODE_MANUAL_RECOVERY is not supported\n");
	} else {
		zassert_equal(err, 0, "failed to set manual-recovery mode (err %d)", err);
		zassert_equal(CAN_MODE_MANUAL_RECOVERY, can_get_mode(can_dev));
	}

	/* restore default CAN mode */
	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set loopback mode (err %d)", err);
	zassert_equal(CAN_MODE_LOOPBACK, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_ERROR_ACTIVE, "CAN controller not started");
}

void *can_classic_setup(void)
{
	can_common_test_setup(CAN_MODE_LOOPBACK);

	return NULL;
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

	/* If previous test has failed due to assertion then CAN may be left stopped.
	 * Tests assume that default CAN state is started.
	 */
	(void)can_start(can_dev);
}

ZTEST_SUITE(can_classic, NULL, can_classic_setup, before, NULL, NULL);
