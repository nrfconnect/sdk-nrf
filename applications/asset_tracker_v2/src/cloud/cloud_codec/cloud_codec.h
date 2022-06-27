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
	/** Contains current cell and number of neighbor cells */
	struct lte_lc_cells_info cell_data;
	/** Contains neighborhood cells */
	struct lte_lc_ncell neighbor_cells[17];
	/** Cell neighborhood data timestamp. UNIX milliseconds. */
	int64_t ts;
	/** Flag signifying that the data entry is to be encoded. */
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
	/** Cloud codec event type */
	enum cloud_codec_event_type type;
	/** New config data if type==CLOUD_CODEC_EVT_CONFIG_UPDATE */
	struct cloud_data_cfg config_update;
};

/** @brief Event handler prototype.
 *
 * @param[in] evt event type
 */
typedef void (*cloud_codec_evt_handler_t)(const struct cloud_codec_evt *evt);

/**
 * @brief Initialize cloud codec.
 * @details Is called once in the data module to initialize the cloud codec used.
 * @param[in] cfg initial config data
 * @param[in] event_handler handler for events coming from the codec
 * 
 * @note currently only used for config updates in lwm2m
 * 
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_init(struct cloud_data_cfg *cfg, cloud_codec_evt_handler_t event_handler);

/**
 * @brief Encode cloud codec neighbor cells data.
 * @details Is called as part of the data_encode() function to encode LTE neighbor cells info.
 * @param[out] output string buffer for encoding result
 * @param[in] neighbor_cells neighbor cells data
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells);

/**
 * @brief Encode cloud codec A-GPS request.
 * 
 * @param[out] output string buffer for encoding result
 * @param[in] agps_request A-GPS request data
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_agps_request(struct cloud_codec_data *output,
				    struct cloud_data_agps_request *agps_request);

/**
 * @brief Encode cloud codec P-GPS request.
 * 
 * @param[out] output string buffer for encoding result
 * @param[in] pgps_request P-GPS request data
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_pgps_request(struct cloud_codec_data *output,
				    struct cloud_data_pgps_request *pgps_request);

/**
 * @brief Decode received configuration.
 * 
 * @param[in] input string buffer with encoded config
 * @param[in] input_len length of input
 * @param[out] cfg decoded config
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_decode_config(char *input, size_t input_len,
			      struct cloud_data_cfg *cfg);

/**
 * @brief Encode current configuration.
 * 
 * @param[out] output string buffer for encoding result
 * @param[in] cfg current configuration
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *cfg);

/**
 * @brief Encode cloud buffer data.
 * 
 * @param[out] output string buffer for encoding result
 * @param[in] data struct with most recent cloud buffer entries
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_data(struct cloud_codec_data *output,
			    struct cloud_codec_data_to_encode data);

/**
 * @brief Encode UI data.
 * 
 * @param[out] output string buffer for encoding result
 * @param[in] ui_buf UI data to encode
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf);

/**
 * @brief Encode a batch of cloud buffer data.
 * 
 * @param[out] output string buffer for encoding result
 * @param[in] data struct with all cloud buffers
 * @return 0 on success or error code according to errno.h. 
 */
int cloud_codec_encode_batch_data(struct cloud_codec_data *output,
				  struct cloud_codec_buffers data);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif
