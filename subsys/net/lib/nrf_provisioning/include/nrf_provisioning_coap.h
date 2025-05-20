/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_provisioning_coap.h
 *
 * @brief nRF Provisioning COAP API.
 */
#ifndef NRF_PROVISIONING_COAP_H__
#define NRF_PROVISIONING_COAP_H__

#include <net/nrf_provisioning.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Parameters and data for using the nRF Cloud CoAP API */
struct nrf_provisioning_coap_context {
	/** Connection socket; initialize to -1 and library
	 * will make the connection.
	 */
	int connect_socket;
	/** User allocated buffer for receiving CoAP API response.
	 * Buffer size should be limited according to the
	 * maximum TLS receive size of the modem.
	 */
	char *rx_buf;
	/** Size of rx_buf */
	size_t rx_buf_len;

	/** Results from API call */
	/** CoAP response of API call */
	int16_t code;
	/** Start of response content data in rx_buf */
	char *response;
	/** Length of response content data */
	size_t response_len;
};

int nrf_provisioning_coap_init(struct nrf_provisioning_mm_change *mmode);
int nrf_provisioning_coap_req(struct nrf_provisioning_coap_context *const coap_ctx);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_COAP_H__ */
