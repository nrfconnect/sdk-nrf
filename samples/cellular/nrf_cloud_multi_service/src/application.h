/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#if defined(CONFIG_NRF_CLOUD_MQTT)
#define MSG_OBJ_DEFINE(_obj_name) \
	NRF_CLOUD_OBJ_JSON_DEFINE(_obj_name);
#elif defined(CONFIG_NRF_CLOUD_COAP)
#define MSG_OBJ_DEFINE(_obj_name) \
	NRF_CLOUD_OBJ_COAP_CBOR_DEFINE(_obj_name);
#endif

/**
 * @brief Main application -- Wait for valid connection, start location tracking, and then
 * periodically sample sensors and send them to nRF Cloud.
 */
void main_application_thread_fn(void);

#endif /* _APPLICATION_H_ */
