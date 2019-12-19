/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _AGGREGATOR_H_
#define _AGGREGATOR_H_

/*
 * Simple aggregator module which can have sensordata added and retrieved in a
 * FIFO fashion. The queue allows TLD formated data to be aggregated from
 * sensors in a FIFO fashion. When some event in the future causes data
 * transfer to start, the most recent X datapoints (determined by ENTRY_MAX_SIZE
 * and HEAP_MEM_POOL_SIZE) can be transferred.
 *
 * Note: The module is single instance to allow simple global access to the
 * data.
 */
#include <zephyr/types.h>
#include <drivers/gps.h>

/* In this sample the GPS data is 81 byte + a data sequence tag,
 * so 85 byte is used.
 */
#define ENTRY_MAX_SIZE (GPS_NMEA_SENTENCE_MAX_LENGTH + 4)
#define FIFO_MAX_ELEMENT_COUNT 12

enum sensor_data_type { THINGY_ORIENTATION, GPS_POSITION };

struct sensor_data {
	u8_t length;
	enum sensor_data_type type;
	u8_t data[ENTRY_MAX_SIZE];
};

int aggregator_init(void);

int aggregator_put(struct sensor_data data);

int aggregator_get(struct sensor_data *data);

void aggregator_clear(void);

int aggregator_element_count_get(void);

#endif /* _AGGREGATOR_H_ */
