/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_MQTT_INTERNAL_H__
#define NRF_CLOUD_MQTT_INTERNAL_H__

#include "nrf_cloud_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Decode the shadow data and get the requested FSM state. */
int nrf_cloud_shadow_data_state_decode(const struct nrf_cloud_obj_shadow_data *const input,
				       enum nfsm_state *const requested_state);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_MQTT_INTERNAL_H__ */
