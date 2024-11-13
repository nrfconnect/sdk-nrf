/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file downloader_transport_http.h
 *
 * @defgroup downloader_transport_http Downloader HTTP transport
 * @ingroup downloader_transport
 * @{
 * @brief Downloader transport HTTP definitions.
 */

#ifndef __DOWNLOADER_TRANSPORT_HTTP_H
#define __DOWNLOADER_TRANSPORT_HTTP_H

#include <net/downloader.h>

/**
 * @brief HTTP transport configuration params.
 */
struct downloader_transport_http_cfg {
	/** Socket receive timeout in milliseconds */
	uint32_t sock_recv_timeo_ms;
};

/**
 * @brief Set Downloader HTTP transport settings
 *
 * @param dl downloader instance
 * @param cfg HTTP transport configuration
 *
 * @return Zero on success, negative errno on failure.
 */
int downloader_transport_http_set_config(struct downloader *dl,
					 struct downloader_transport_http_cfg *cfg);

#endif /* __DOWNLOADER_TRANSPORT_HTTP_H */

/**@} */
