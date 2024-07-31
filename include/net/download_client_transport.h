/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DOWNLOAD_CLIENT_TRANSPORT_H
#define DOWNLOAD_CLIENT_TRANSPORT_H

#include <net/download_client.h>

int dlc_transport_evt_connected(struct download_client *dlc);
int dlc_transport_evt_disconnected(struct download_client *dlc);
int dlc_transport_evt_data(struct download_client *dlc, void *data, size_t len);
int dlc_transport_evt_download_complete(struct download_client *dlc);

struct dlc_transport {
	//TODO Consider whether the transports should not have access to the DLC struct, but get the parameters they need?
	/**
	 * Parse protocol
	 *
	 */
	bool (*proto_supported)(struct download_client *dlc, const char *uri);
	/**
	 * Initialize DLC transport
	 *
	 * @param ...
	 * @returns 0 on success, negative error on failure.
	 */
	int (*init)(struct download_client *dlc, struct download_client_host_cfg *host_cgf, const char *uri);
	/**
	 * Deinitialize DLC transport
	 *
	 * @param ...
	 * @returns 0 on success, negative error on failure.
	 */
	int (*deinit)(struct download_client *dlc);
	/**
	 * Connect DLC transport.
	 *
	 * Connection result is given by callback to @c dlc_transport_event_connected.
	 *
	 * @param ...
	 * @returns 0 on success, negative error on failure.
	 */
	int (*connect)(struct download_client *dlc);
	/**
	 * Close DLC transport
	 *
	 * @param ...
	 * @returns 0 on success, negative error on failure.
	 */
	int (*close)(struct download_client *dlc);
	/**
	 * Download data with DLC transport
	 *
	 * @param ...
	 * @returns 0 on success, negative error on failure.
	 */
	int (*download)(struct download_client *dlc);
};

struct dlc_transport_entry {
	struct dlc_transport *transport;
};

/**
 * @brief Define a DLC transport.
 *
 * @param entry The entry name.
 * @param _transport The transport.
 */
#define DLC_TRANSPORT(entry, _transport)                                                           \
	static STRUCT_SECTION_ITERABLE(dlc_transport_entry, entry) = {                             \
		.transport = _transport,                                                           \
	}

#endif /* DOWNLOAD_CLIENT_TRANSPORT_H */
