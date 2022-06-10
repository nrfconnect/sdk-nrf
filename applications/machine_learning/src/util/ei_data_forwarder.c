/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "ei_data_forwarder.h"
#include <zephyr/drivers/sensor.h>


static int snprintf_error_check(int res, size_t buf_size)
{
	if (res < 0) {
		return res;
	} else if (res >= buf_size) {
		return -ENOBUFS;
	}

	return 0;
}

int ei_data_forwarder_parse_data(const struct sensor_value *data_ptr, size_t data_cnt,
				 char *buf, size_t buf_size)
{
	int pos = 0;

	for (size_t i = 0; i < 2 * data_cnt; i++) {
		int tmp;

		if ((i % 2) == 0) {
			tmp = snprintf(&buf[pos], buf_size - pos, "%.2f", sensor_value_to_double(&data_ptr[i / 2]));
		} else if (i == (2 * data_cnt - 1)) {
			tmp = snprintf(&buf[pos], buf_size - pos, "\r\n");
		} else {
			tmp = snprintf(&buf[pos], buf_size - pos, ",");
		}

		int err = snprintf_error_check(tmp, buf_size - pos);

		if (err) {
			pos = err;
			break;
		}
		pos += tmp;
	}

	return pos;
}
