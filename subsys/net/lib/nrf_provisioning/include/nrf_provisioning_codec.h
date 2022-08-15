/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_PROVISIONING_CODEC_H__
#define NRF_PROVISIONING_CODEC_H__

#include <modem/lte_lc.h>

#include "nrf_provisioning_cbor_encode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Max length of a correlation ID */
#define NRF_PROVISIONING_CORRELATION_ID_SIZE sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.yyyyyyyy")
/** Key used to identify the latest 'FINISHED' provisioning command ID in the settings */
#define NRF_PROVISIONING_CORRELATION_ID_KEY "after-correlation-id"

/** Value telling that provisioning is finished, any positive value would do */
#define NRF_PROVISIONING_FINISHED 1

/** @brief Input and output data for the coded. */
struct cdc_context {
	char *ipkt; /* Application protocol cmd response payload */
	size_t ipkt_sz; /* Payload length */
	void *i_data; /* Decoded commands */
	size_t i_data_sz; /* Size of decoded commands */
	char *opkt; /* Application protocol response request payload */
	size_t opkt_sz; /* Encoded responses' size */
	void *o_data; /* Raw responses */
	size_t o_data_sz; /* Size of decoded commands */
};

/**
 * @typedef nrf_provisioning_codec_mm_cb_t
 * @brief Callback to request a modem state change, being it powering off, flight mode etc.
 *
 * @return <0 on error,  previous mode on success.
 */
typedef int (*nrf_provisioning_codec_mm_cb_t)(enum lte_lc_func_mode new_mode, void *user_data);

/**
 * @struct nrf_provisioning_codec_mm_change
 * @brief Callback used for querying permission from the app to proceed when modem's state changes
 *
 * @param cb        The callback function.
 * @param user_data App specific data to be fed to the callback once it's called.
 */
struct nrf_provisioning_codec_mm_change {
	nrf_provisioning_codec_mm_cb_t cb;
	void *user_data;
};

/**
 *  @brief Initialize the codec used encoding the data to the cloud.
 */
int nrf_provisioning_codec_init(struct nrf_provisioning_codec_mm_change *mmode);

/**
 * @brief For each provisioning request setup the environment
 */
int nrf_provisioning_codec_setup(struct cdc_context *const cdc_ctx,
	char * const at_buff, size_t at_buff_sz);

/**
 * @brief Release allocated resources.
 *
 * @return <0 on error,  0 on success.
 */
int nrf_provisioning_codec_teardown(void);

/**
 * @brief Decodes and processes received provisioning commands. Encodes the responses.
 *
 * @return <0 on error,  0 on success and >0 when finished with provisioning.
 */
int nrf_provisioning_codec_process_commands(void);

/**
 * @brief Get the latest successfully executed provisioning command's correlation ID.
 *
 * @return Pointer to the correlation ID.
 */
char * const nrf_provisioning_codec_get_latest_cmd_id(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_CODEC_H__ */
