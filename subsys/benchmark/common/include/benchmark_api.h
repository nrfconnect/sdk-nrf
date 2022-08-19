/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BENCHMARK_H__
#define BENCHMARK_H__

#include <stdbool.h>
#include <stdint.h>

#include "timer_api.h"

#define BENCHMARK_MAX_PEER_NUMBER                                                                  \
	16 /**< Maximal number of discovered peers that can be stored in the database. */
#define BENCHMARK_SCHED_MAX_EVENT_DATA_SIZE                                                        \
	52 /**< Maximum possible size of a data structure passed by the app scheduler module. */
#define BENCHMARK_DISCOVERY_TIMEOUT                                                                \
	1000 /**< Time after which application stops waiting for discovery responses in ms */
#define BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED                                                     \
	0xFFFFFFFF /* Counters value indicating that this statistic measurement is not supported. */

enum benchmark_test_state {
	/**< Frame has been scheduled for sending and no acknowledgment is expected. */
	TEST_IN_PROGRESS_FRAME_SENDING,
	/**< Frame has been just sent. */
	TEST_IN_PROGRESS_FRAME_SENT,
	/**< Frame has been just sent and acknowledgment is expected. */
	TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK,
	/**< Frame has been just sent and echo reply is expected. */
	TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO,
	/**< Frame has been already sent and we are still waiting for the acknowledgment. */
	TEST_IN_PROGRESS_WAITING_FOR_ACK,
	/**< No more messages can be appended to the transmission buffer at this time. */
	TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER,
	/**< Test started in slave mode. Waiting for "test stop" CLI command. */
	TEST_IN_PROGRESS_WAITING_FOR_STOP_CMD,
	/**< No test in progress. */
	TEST_IDLE,
	/**< Application error occurred. */
	TEST_ERROR,
	/**< Test has just finished. */
	TEST_FINISHED,
};

/**< Forward declaration of a protocol-specific address structure. */
struct benchmark_address_context;

struct benchmark_peer_entry {
	struct benchmark_address_context *p_address; /**< Pointer to protocol-specific data. */
	uint64_t device_id; /**< Device ID read from FICR. */
};

struct benchmark_peer_db {
	/**< Number of found peers. */
	uint16_t peer_count;
	/**< Index of the currently selected peer */
	uint16_t selected_peer;
	/**< Table of found peers. */
	struct benchmark_peer_entry peer_table[BENCHMARK_MAX_PEER_NUMBER];
};

enum benchmark_mode {
	/**< Transmitter will just send packets to receiver in one direction. */
	BENCHMARK_MODE_UNIDIRECTIONAL = 0,
	/**< Receiver replies with the same data back to the transmitter. */
	BENCHMARK_MODE_ECHO,
	/**< Receiver replies with a short information that the data has been received. */
	BENCHMARK_MODE_ACK,
};

struct benchmark_configuration {
	uint16_t length; /**< Length of the protocol-specific payload. */
	uint16_t ack_timeout; /**< Timeout, in milliseconds, used in ACK and ECHO modes */
	uint32_t count; /**< Number of packets to send. */
	enum benchmark_mode mode; /**< Receiver mode. */
};

struct benchmark_latency {
	uint32_t min; /**< Minimum measured latency [us]. */
	uint32_t max; /**< Maximum measured latency [us]. */

	uint32_t cnt; /**< Number of summed latencies. */
	uint64_t sum; /**< Sum of measured latencies. */
};

struct benchmark_status {
	bool test_in_progress; /**< Indicates that the test is ongoing. */
	bool reset_counters; /**< Reset counters upon reception of the first packet. */
	uint32_t acks_lost; /**< Total number of acknowledgments lost. */
	uint32_t waiting_for_ack; /**< Frame number for which acknowledgment is awaited. */
	uint32_t packets_left_count; /**< Number of sent packets. */
	uint32_t frame_number; /**< Number of the test frame included in the payload. */
	struct benchmark_latency latency; /**< Measured latency values. */
};

struct benchmark_rx_counters {
	/**< Total bytes of the protocol-specific packet payload received. */
	uint32_t bytes_received;
	/**< Total number of test packets received. */
	uint32_t packets_received;
	/**< Total number of incorrectly received frames. */
	uint32_t rx_error;
	/**< Total number of correctly received frames. */
	uint32_t rx_total;
};

struct benchmark_mac_counters {
	uint32_t total; /**< Total number of attempts. */
	uint32_t error; /**< Total number of failed attempts. */
};

struct benchmark_result {
	/**< CPU utilization [0.01%]. */
	uint32_t cpu_utilization;
	/**< Test duration [ms]. */
	uint32_t duration;
	/**< Counters from peer that are tx-ed to the local node after the test. */
	struct benchmark_rx_counters rx_counters;
	/**< MAC transmit counters. */
	struct benchmark_mac_counters mac_tx_counters;
};

struct benchmark_evt_results {
	/**< Pointer to the test result structure, measured on the remote peer. */
	struct benchmark_result *p_remote_result;
	/**< Pointer to the test result structure, measured on the local node. */
	struct benchmark_result *p_local_result;
	/**< Pointer to the test status on the local node. */
	struct benchmark_status *p_local_status;
};

union benchmark_event_context {
	/**< Structure with pointers to both: local and remote results. */
	struct benchmark_evt_results results;
	/**< Pointer to the peer information. */
	struct benchmark_peer_db *p_peer_information;
	/**< Error related to the last operation. */
	uint32_t error;
};

enum benchmark_event {
	/**< Test completed, takes place after results reception and throughput/per calculation. */
	BENCHMARK_TEST_COMPLETED = 0,
	/**< Test started, transmission is starting. */
	BENCHMARK_TEST_STARTED,
	/**< Test stopped, peer has been sent the test stop control message. */
	BENCHMARK_TEST_STOPPED,
	/**< Discovery completed, local node should receive discovery responses by now. */
	BENCHMARK_DISCOVERY_COMPLETED,
};

struct benchmark_evt {
	 /**< Benchmark event. */
	enum benchmark_event evt;
	/**< Additional event information to be used in application. */
	union benchmark_event_context context;
};

/**@brief Type definition of the benchmark callback.
 *
 * @param[in] p_evt Pointer to the event structure.
 */
typedef void (*benchmark_callback_t)(struct benchmark_evt *p_evt);

/**************************************************************************************************
 * Public API.
 *************************************************************************************************/

/**@brief   Function for initializing the communication used for the peer test control. */
void benchmark_init(void);

/**@brief   Function for initializing the temporary test setting before the run.
 *
 * @param[in] callback    Callback function to asynchronously inform the application
 *                        about the benchmark state.
 *
 * @returns Protocol-specific error code.
 */
uint32_t benchmark_test_init(struct benchmark_configuration *p_configuration,
			     benchmark_callback_t callback);

/**@brief   Function for starting current test configuration.
 *
 * @returns Protocol-specific error code.
 */
uint32_t benchmark_test_start(void);

/**@brief   Function for starting current test configuration.
 *
 * @returns Protocol-specific error code.
 */
uint32_t benchmark_test_stop(void);

/**@brief   Function for discovering peers that can be used as receiver during the test.
 *
 * @returns Protocol-specific error code
 */
uint32_t benchmark_peer_discover(void);

/**@brief   Function that returns table pointer of found peers which can serve
 *          as the receiver during the test.
 *
 * @retval A pointer to the benchmark peer information structure.
 */
const struct benchmark_peer_db *benchmark_peer_table_get(void);

/**@brief   Function that returns test status structure pointer.
 *
 * @retval A pointer to the benchmark status structure.
 */
struct benchmark_status *benchmark_status_get(void);

/**@brief   Function that processes the test. */
void benchmark_process(void);

/**@brief   Function that returns Device ID read from FICR.
 *
 * @retval Device ID read from the FICR register.
 */
uint64_t benchmark_local_device_id_get(void);

/**@brief   Function to send result request to the peer.
 *
 * @returns Protocol-specific error code.
 */
uint32_t benchmark_peer_results_request_send(void);

/**@brief   Function returning selected peer pointer.
 *
 * @retval A pointer to the benchmark peer entry structure of the selected peer.
 */
const struct benchmark_peer_entry *benchmark_peer_selected_get(void);

/**@brief   Function that clears latency measurement.
 *
 * @param[out] p_latency    Pointer to latency measurements to be cleared.
 */
void benchmark_clear_latency(struct benchmark_latency *p_latency);

/**@brief   Function that adds a new value to latency measurement.
 *
 * @param[inout] p_latency    Pointer to latency measurements to be updated.
 * @param[in]    latency      Currently measured latency.
 */
void benchmark_update_latency(struct benchmark_latency *p_latency, uint32_t latency);

#endif // BENCHMARK_H__
