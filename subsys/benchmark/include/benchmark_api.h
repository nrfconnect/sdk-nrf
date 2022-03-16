/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/
/**@cond To Make Doxygen skip documentation generation for this file.
 * @{
 */

#ifndef BENCHMARK_H__
#define BENCHMARK_H__

#include <stdbool.h>
#include <stdint.h>

#include "stub_api.h"

#define BENCHMARK_MAX_PEER_NUMBER               16         /**< Maximal number of discovered peers that can be stored in the database. */
#define BENCHMARK_SCHED_MAX_EVENT_DATA_SIZE     52         /**< Maximum possible size of a data structure passed by the app scheduler module. */
#define BENCHMARK_DISCOVERY_TIMEOUT             1000       /**< Time after which application stops waiting for discovery responses in ms */
#define BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED  0xFFFFFFFF /* Counters value indicating that this statistic measurement is not supported. */

typedef enum
{
    TEST_IN_PROGRESS_FRAME_SENDING,               /**< Frame has been scheduled for sending and no acknowledgment is expected. */
    TEST_IN_PROGRESS_FRAME_SENT,                  /**< Frame has been just sent. */
    TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK,  /**< Frame has been just sent and acknowledgment is expected. */
    TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO, /**< Frame has been just sent and echo reply is expected. */
    TEST_IN_PROGRESS_WAITING_FOR_ACK,             /**< Frame has been already sent and we are still waiting for the acknowledgment. */
    TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER,       /**< No more messages can be appended to the transmission buffer at this time. */
    TEST_IN_PROGRESS_WAITING_FOR_STOP_CMD,        /**< Test started in slave mode. Waiting for "test stop" CLI command. */
    TEST_IDLE,                                    /**< No test in progress. */
    TEST_ERROR,                                   /**< Application error occurred. */
    TEST_FINISHED,                                /**< Test has just finished. */
} benchmark_test_state_t;

typedef struct benchmark_address_context_t benchmark_address_context_t; /**< Forward declaration of a protocol-specific address structure. */

typedef struct
{
    benchmark_address_context_t * p_address;    /**< Pointer to protocol-specific data. */
    uint64_t                      device_id;    /**< Device ID read from FICR. */
} benchmark_peer_entry_t;

typedef struct
{
    uint16_t               peer_count;                             /**< Number of found peers. */
    uint16_t               selected_peer;                          /**< Index of the currently selected peer */
    benchmark_peer_entry_t peer_table[BENCHMARK_MAX_PEER_NUMBER];  /**< Table of found peers. */
} benchmark_peer_db_t;

typedef enum
{
    BENCHMARK_MODE_UNIDIRECTIONAL = 0, /**< Transmitter will just send packets to receiver in one direction. */
    BENCHMARK_MODE_ECHO,               /**< Receiver replies with the same data back to the transmitter. */
    BENCHMARK_MODE_ACK,                /**< Receiver replies with a short information that the data has been received. */
} benchmark_mode_t;

typedef struct
{
    uint16_t         length;       /**< Length of the protocol-specific payload. */
    uint16_t         ack_timeout;  /**< Timeout, in milliseconds, used in ACK and ECHO modes */
    uint32_t         count;        /**< Number of packets to send. */
    benchmark_mode_t mode;         /**< Receiver mode. */
} benchmark_configuration_t;

typedef struct
{
    uint32_t min; /**< Minimum measured latency [us]. */
    uint32_t max; /**< Maximum measured latency [us]. */

    uint32_t cnt; /**< Number of summed latencies. */
    uint64_t sum; /**< Sum of measured latencies. */
} benchmark_latency_t;

typedef struct
{
    bool                test_in_progress;   /**< Indicates that the test is ongoing. */
    bool                reset_counters;     /**< Reset counters upon reception of the first packet. */
    uint32_t            acks_lost;          /**< Total number of acknowledgments lost. */
    uint32_t            waiting_for_ack;    /**< Frame number for which acknowledgment is awaited. */
    uint32_t            packets_left_count; /**< Number of sent packets. */
    uint32_t            frame_number;       /**< Number of the test frame included in the payload. */
    benchmark_latency_t latency;            /**< Measured latency values. */
} benchmark_status_t;

typedef struct
{
    uint32_t bytes_received;   /**< Total bytes of the protocol-specific packet payload received [B]. */
    uint32_t packets_received; /**< Total number of test packets received. */
    uint32_t rx_error;         /**< Total number of incorrectly received frames. */
    uint32_t rx_total;         /**< Total number of correctly received frames. */
} benchmark_rx_counters_t;

typedef struct
{
    uint32_t total; /**< Total number of attempts. */
    uint32_t error; /**< Total number of failed attempts. */
} benchmark_mac_counters_t;

typedef struct
{
    uint32_t                 cpu_utilization; /**< CPU utilization [0.01%]. */
    uint32_t                 duration;        /**< Test duration [ms]. */
    benchmark_rx_counters_t  rx_counters;     /**< Counter values from peer that are transferred to the local node after the test. */
    benchmark_mac_counters_t mac_tx_counters; /**< MAC transmit counters. */
} benchmark_result_t;

typedef struct
{
    benchmark_result_t  * p_remote_result;    /**< Pointer to the test result structure, measured on the remote peer. */
    benchmark_result_t  * p_local_result;     /**< Pointer to the test result structure, measured on the local node. */
    benchmark_status_t  * p_local_status;     /**< Pointer to the test status on the local node. */
} benchmark_evt_results_t;

typedef union
{
    benchmark_evt_results_t   results;            /**< Structure with pointers to both: local and remote results. */
    benchmark_peer_db_t     * p_peer_information; /**< Pointer to the peer information. */
    uint32_t                  error;              /**< Error related to the last operation. */
} benchmark_event_context_t;

typedef enum
{
    BENCHMARK_TEST_COMPLETED = 0,  /**< Test completed event, takes place after results reception and throughput/per calculation. */
    BENCHMARK_TEST_STARTED,        /**< Test started event, transmission is starting. */
    BENCHMARK_TEST_STOPPED,        /**< Test stopped event, peer has been sent the test stop control message. */
    BENCHMARK_DISCOVERY_COMPLETED, /**< Discovery completed event, local node should receive discovery responses from available peers by now. */
} benchmark_event_t;

typedef struct
{
    benchmark_event_t         evt;     /**< Benchmark event. */
    benchmark_event_context_t context; /**< Additional event information to be used in application. */
} benchmark_evt_t;


/**@brief Type definition of the the benchmark callback.
 *
 * @param[in] p_evt Pointer to the event structure.
 */
typedef void (*benchmark_callback_t)(benchmark_evt_t * p_evt);

/**************************************************************************************************
 * Public API.
 *************************************************************************************************/

/**@brief   Function for initializing the communication used for the peer test control. */
void benchmark_init(void);

/**@brief   Function for initializing the temporary test setting before the run.
 *
 * @param[in] callback    Callback function to asynchronously inform the application about the benchmark state.
 *
 * @returns Protocol-specific error code.
 */
uint32_t benchmark_test_init(benchmark_configuration_t * p_configuration, benchmark_callback_t callback);

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

/**@brief   Function that returns table pointer of found peers which can serve as the receiver during the test.
 *
 * @retval A pointer to the benchmark peer information structure.
 */
const benchmark_peer_db_t * benchmark_peer_table_get(void);

/**@brief   Function that returns test status structure pointer.
 *
 * @retval A pointer to the benchmark status structure.
 */
benchmark_status_t * benchmark_status_get(void);

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
const benchmark_peer_entry_t * benchmark_peer_selected_get(void);

/**@brief   Function that clears latency measurement.
 *
 * @param[out] p_latency    Pointer to latency measurements to be cleared.
 */
void benchmark_clear_latency(benchmark_latency_t * p_latency);

/**@brief   Function that adds a new value to latency measurement.
 *
 * @param[inout] p_latency    Pointer to latency measurements to be updated.
 * @param[in]    latency      Currently measured latency.
 */
void benchmark_update_latency(benchmark_latency_t * p_latency, uint32_t latency);

#endif // BENCHMARK_H__

/** @}
 *  @endcond
 */
