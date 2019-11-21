/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NRF_CLOUD_CODEC_H__
#define NRF_CLOUD_CODEC_H__

#include <nrf_cloud.h>
#include "nrf_cloud_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initialize the codec used encoding the data to the cloud. */
int nrf_codec_init(void);

/**@brief Encode the sensor data based on the indicated type. */
int nrf_cloud_encode_sensor_data(const struct nrf_cloud_sensor_data *input,
				 struct nrf_cloud_data *output);

/**@brief Encode the sensor data to be sent to the device shadow. */
int nrf_cloud_encode_shadow_data(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output);

/**@brief Encode the user association data based on the indicated type. */
int nrf_cloud_decode_requested_state(const struct nrf_cloud_data *payload,
				     enum nfsm_state *requested_state);

/**@brief Decodes data endpoint information. */
int nrf_cloud_decode_data_endpoint(const struct nrf_cloud_data *input,
				   struct nrf_cloud_data *tx_endpoint,
				   struct nrf_cloud_data *rx_endpoint,
				   struct nrf_cloud_data *m_endpoint);

/** @brief Encodes state information. */
int nrf_cloud_encode_state(u32_t reported_state, struct nrf_cloud_data *output);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CODEC_H__ */
