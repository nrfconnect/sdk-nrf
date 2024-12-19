/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file downloader_transport_coap.h
 *
 * @defgroup downloader_transport_http Downloader HTTP transport
 * @ingroup downloader_transport
 * @{
 * @brief Downloader transport CoAP definitions.
 */

#ifndef __DOWNLOADER_TRANSPORT_COAP_H
#define __DOWNLOADER_TRANSPORT_COAP_H

#include <net/downloader.h>
#include <zephyr/net/coap.h>

/**
 * @brief CoAP transport configuration params.
 */
struct downloader_transport_coap_cfg {
	/** CoAP block size. */
	enum coap_block_size block_size;
	/** Max retransmission requests. */
	uint8_t max_retransmission;
};

/**
 * @brief Set Downloader CoAP transport settings
 *
 * @param dl downloader instance
 * @param cfg CoAP transport configuration
 *
 * @return Zero on success, negative errno on failure.
 */
int downloader_transport_coap_set_config(struct downloader *dl,
					 struct downloader_transport_coap_cfg *cfg);

#endif /* __DOWNLOADER_TRANSPORT_COAP_H */

/**@} */
