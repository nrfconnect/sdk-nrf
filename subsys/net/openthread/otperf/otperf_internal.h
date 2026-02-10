/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __OTPERF_INTERNAL_H
#define __OTPERF_INTERNAL_H

#include <limits.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>
#include <openthread/ip6.h>

#define OTPERF_FLAGS_VERSION1 0x80000000

#define DEF_PORT_STR		 STRINGIFY(CONFIG_OTPERF_DEFAULT_PORT)
#define DEF_DURATION_SECONDS_STR STRINGIFY(CONFIG_OTPERF_DEFAULT_DURATION_S)
#define DEF_PACKET_SIZE_STR	 STRINGIFY(CONFIG_OTPERF_DEFAULT_PACKET_SIZE)
#define DEF_RATE_KBPS_STR	 STRINGIFY(CONFIG_OTPERF_DEFAULT_RATE_KBPS)

#define OTPERF_VERSION "1.0"

struct otperf_udp_datagram {
	uint32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;
	uint32_t id2;
} __packed;

struct otperf_client_hdr_v1 {
	int32_t flags;
	int32_t num_of_threads;
	int32_t port;
	int32_t buffer_len;
	int32_t bandwidth;
	int32_t num_of_bytes;
};

struct otperf_server_hdr {
	int32_t flags;
	int32_t total_len1;
	int32_t total_len2;
	int32_t stop_sec;
	int32_t stop_usec;
	int32_t error_cnt;
	int32_t outorder_cnt;
	int32_t datagrams;
	int32_t jitter1;
	int32_t jitter2;
};

enum otperf_status {
	OTPERF_SESSION_STARTED,
	OTPERF_SESSION_FINISHED,
	OTPERF_SESSION_ERROR
} __packed;

struct otperf_upload_params {
	uint64_t unix_offset_us;
	struct otSockAddr peer_addr;
	uint32_t duration_ms;
	uint32_t rate_kbps;
	uint16_t packet_size;
};

struct otperf_download_params {
	struct otSockAddr addr;
};

/** Performance results */
struct otperf_results {
	uint32_t nb_packets_sent;     /**< Number of packets sent */
	uint32_t nb_packets_rcvd;     /**< Number of packets received */
	uint32_t nb_packets_lost;     /**< Number of packets lost */
	uint32_t nb_packets_outorder; /**< Number of packets out of order */
	uint64_t total_len;	      /**< Total length of the transferred data */
	uint64_t time_in_us;	      /**< Total time of the transfer in microseconds */
	uint32_t jitter_in_us;	      /**< Jitter in microseconds */
	uint64_t client_time_in_us;   /**< Client connection time in microseconds */
	uint32_t packet_size;	      /**< Packet size */
	uint32_t nb_packets_errors;   /**< Number of packet errors */
};

/**
 * @brief Otperf callback function used for asynchronous operations.
 *
 * @param status Session status.
 * @param result Session results. May be NULL for certain events.
 * @param user_data A pointer to the user provided data.
 */
typedef void (*otperf_callback)(enum otperf_status status, struct otperf_results *result,
				void *user_data);

/**
 * @brief Synchronous UDP upload operation. The function blocks until the upload
 *        is complete.
 *
 * @param param Upload parameters.
 * @param result Session results.
 *
 * @return 0 if session completed successfully, a negative error code otherwise.
 */
int otperf_udp_upload(const struct otperf_upload_params *param, struct otperf_results *result);

/**
 * @brief Start UDP server.
 *
 * @note Only one UDP server instance can run at a time.
 *
 * @param param Download parameters.
 * @param callback Session results callback.
 * @param user_data A pointer to the user data to be provided with the callback.
 *
 * @return 0 if server was started, a negative error code otherwise.
 */
int otperf_udp_download(const struct otperf_download_params *param, otperf_callback callback,
			void *user_data);

/**
 * @brief Stop UDP server.
 *
 * @return 0 if server was stopped successfully, a negative error code otherwise.
 */
int otperf_udp_download_stop(void);

static inline uint32_t time_delta(uint32_t ts, uint32_t t)
{
	return (t >= ts) ? (t - ts) : (UINT32_MAX - ts + t);
}

uint32_t otperf_packet_duration(uint32_t packet_size, uint32_t rate_in_kbps);

#endif /* __OTPERF_INTERNAL_H */
