/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_CODEC_H__
#define CLOUD_CODEC_H__

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cJSON_os.h>
#include "generated_cloud_buffers.h"

/**@file
 *
 * @defgroup cloud_codec Cloud codec
 * @brief    Module that encodes and decodes cloud communication.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Structure containing boolean variables used to enable/disable inclusion of the corresponding
 *  data type in sample requests sent out by the application module.
 */
struct cloud_data_no_data {
	/** If this flag is set GNSS data is not included in sample requests. */
	bool gnss;

	/** If this flag is set neighbor cell data is not included sample requests. */
	bool neighbor_cell;
};

struct cloud_data_cfg {
	/** Device mode. */
	bool active_mode;
	/** GNSS search timeout. */
	int gnss_timeout;
	/** Time between cloud publications in Active mode. */
	int active_wait_timeout;
	/** Time between cloud publications in Passive mode. */
	int movement_resolution;
	/** Time between cloud publications regardless of movement
	 *  in Passive mode.
	 */
	int movement_timeout;
	/** Accelerometer trigger threshold value in m/s2. */
	double accelerometer_threshold;
	/** Variable used to govern what data types are requested by the application. */
	struct cloud_data_no_data no_data;
};

struct cloud_codec_data {
	/** Encoded output. */
	char *buf;
	/** Length of encoded output. */
	size_t len;
	/** LwM2M object paths. */
	char paths[CONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX]
		  [CONFIG_CLOUD_CODEC_LWM2M_PATH_ENTRY_SIZE_MAX];
	/* Number of valid paths in the paths variable. */
	uint8_t valid_object_paths;
};

struct cloud_data_neighbor_cells {
	struct lte_lc_cells_info cell_data;
	struct lte_lc_ncell neighbor_cells[17];
	int64_t ts;
	bool queued : 1;
};

struct cloud_data_agps_request {
	/** Mobile Country Code */
	int mcc;
	/** Mobile Network Code */
	int mnc;
	/** Cell ID */
	uint32_t cell;
	/** Area Code */
	uint32_t area;
	/** AGPS request types */
	struct nrf_modem_gnss_agps_data_frame request;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
	/** Flag indicating that the ephemerides will only include visible satellites */
	bool filtered : 1;
	/** Angle above the horizon to constrain the filtered set to. */
	uint8_t mask_angle;

};
struct cloud_data_pgps_request {
	/** Number of requested predictions. */
	uint16_t count;
	/** Time in between predictions, in minutes. */
	uint16_t interval;
	/** The day to start the prediction from. Days since GPS epoch. */
	uint16_t day;
	/** The start time of the prediction in seconds in a day. */
	uint32_t time;
	/** Flag signifying that the data entry is to be encoded. */
	bool queued : 1;
};

enum cloud_codec_event_type {
	CLOUD_CODEC_EVT_CONFIG_UPDATE = 1,
};

struct cloud_codec_evt {
	enum cloud_codec_event_type type;
	struct cloud_data_cfg config_update;
};

typedef void (*cloud_codec_evt_handler_t)(const struct cloud_codec_evt *evt);

int cloud_codec_init(struct cloud_data_cfg *cfg, cloud_codec_evt_handler_t event_handler);

int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells);

int cloud_codec_encode_agps_request(struct cloud_codec_data *output,
				    struct cloud_data_agps_request *agps_request);

int cloud_codec_encode_pgps_request(struct cloud_codec_data *output,
				    struct cloud_data_pgps_request *pgps_request);

int cloud_codec_decode_config(char *input, size_t input_len,
			      struct cloud_data_cfg *cfg);

int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *cfg);

int cloud_codec_encode_data(struct cloud_codec_data *output,
			    struct cloud_codec_data_to_encode data);

int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf);

int cloud_codec_encode_batch_data(struct cloud_codec_data *output,
				  struct cloud_codec_buffers data);

static inline void cloud_codec_release_data(struct cloud_codec_data *output)
{
	cJSON_FreeString(output->buf);
}

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif
