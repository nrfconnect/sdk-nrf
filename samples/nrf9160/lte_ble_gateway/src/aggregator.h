/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
#include <nrf_modem_gnss.h>

/* In this sample the GPS data is 83 byte + a data sequence tag,
 * so 87 byte is used.
 */
#define ENTRY_MAX_SIZE (NRF_MODEM_GNSS_NMEA_MAX_LEN + 4)
#define FIFO_MAX_ELEMENT_COUNT 12

enum sensor_data_type { THINGY_ORIENTATION, GNSS_POSITION };

struct sensor_data {
	uint8_t length;
	enum sensor_data_type type;
	uint8_t data[ENTRY_MAX_SIZE];
};

int aggregator_init(void);

int aggregator_put(struct sensor_data data);

int aggregator_get(struct sensor_data *data);

void aggregator_clear(void);

int aggregator_element_count_get(void);

#endif /* _AGGREGATOR_H_ */
