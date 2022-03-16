/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef BENCHMARK_ZIGBEE_COMMON_H__
#define BENCHMARK_ZIGBEE_COMMON_H__

#include <zboss_api.h>


/**@brief Benchmark control command type definition. */
typedef enum
{
    TEST_START_REQUEST    = 0x00, /**< Notify the receiver about the intention to start the test. */
    TEST_STOP_REQUEST     = 0x01, /**< Notify the receiver about the intention to stop the test. */
    TEST_RESULTS_REQUEST  = 0x02, /**< Notify the receiver about the intention to receive test results. */
    TEST_RESULTS_RESPONSE = 0x03, /**< Notify the receiver that the payload contains remote peer's test results. */
} zigbee_benchmark_ctrl_t;


struct benchmark_address_context_t
{
    zb_uint16_t nwk_addr; /**< Remember only network address. EUI64 address is stored as device_id inside global device table. */
};


/**************************************************************************************************
 * Internal API.
 *************************************************************************************************/

/**@brief Function for stopping benchmark test in the slave mode.
 *
 * @return One of the ZCL statuses that will be sent inside the ZCL Default Response packet.
 */
zb_zcl_status_t zigbee_benchmark_test_stop_slave(void);

/**@brief Function for starting benchmark test in the slave mode.
 *
 * @return One of the ZCL statuses that will be sent inside the ZCL Default Response packet.
 */
zb_zcl_status_t zigbee_benchmark_test_start_slave(void);


/**@brief Function that is called upon the reception of remote test results.
 *
 * @param[in] p_result  A pointer to the structure that contains the remote peer test results.
 */
void zigbee_benchmark_results_received(benchmark_result_t * p_result);

/**@brief Function that is called upon the reception of benchmark control command response.
 *
 * @param[in] cmd_id  ID of the control command that was sent.
 * @param[in] status  ZCL status code.
 */
void zigbee_benchmark_command_response_handler(zigbee_benchmark_ctrl_t cmd_id, zb_zcl_status_t status);

/**@brief Function for aborting the current benchmark test execution. */
void zigbee_benchmark_test_abort(void);

/**@brief Function for returning test result structure pointer.
 *
 * @retval Pointer to the benchmark result structure.
 */
benchmark_result_t * zigbee_benchmark_local_result_get(void);

/**@brief Function for sending test result request to the remote peer.
 *
 * @param[in] bufid     Reference number for the ZBOSS memory buffer.
 * @param[in] nwk_addr  Network address of the remote peer.
 */
void zigbee_benchmark_peer_results_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr);

/**@brief Function for sending test start request to the remote peer.
 *
 * @param[in] bufid     Reference number for the ZBOSS memory buffer.
 * @param[in] nwk_addr  Network address of the remote peer.
 */
void zigbee_benchmark_peer_start_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr);

/**@brief Function for sending test stop request to the remote peer.
 *
 * @param[in] bufid     Reference number for the ZBOSS memory buffer.
 * @param[in] nwk_addr  Network address of the remote peer.
 */
void zigbee_benchmark_peer_stop_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr);

#endif /* BENCHMARK_ZIGBEE_COMMON_H__ */
