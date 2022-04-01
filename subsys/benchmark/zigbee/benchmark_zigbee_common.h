/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef BENCHMARK_ZIGBEE_COMMON_H__
#define BENCHMARK_ZIGBEE_COMMON_H__

#include <zboss_api.h>


/**@brief Benchmark control command type definition. */
typedef enum
{
    TEST_START_REQUEST        = 0x00, /**< Notify the receiver about the intention to start the test. */
    TEST_STOP_REQUEST         = 0x01, /**< Notify the receiver about the intention to stop the test. */
    TEST_RESULTS_REQUEST      = 0x02, /**< Notify the receiver about the intention to receive test results. */
    TEST_RESULTS_RESPONSE     = 0x03, /**< Notify the receiver that the payload contains remote peer's test results. */
    TEST_SET_TX_POWER         = 0x04, /**< Notify the received about the intention to change its tx power. */
    TEST_GET_TX_POWER         = 0x05, /**< Notify the received about the intention to get its tx power. */
    TEST_TX_POWER_RESPONSE    = 0x06, /**< Provide the receiver with a current configuration of sender's tx power. */
    TEST_OPEN_NETWORK_REQUEST = 0x07, /**< Request the receiver to open the network for joining, doesn't have any effect
                                           if the receiver isn't the network coordinator */
    TEST_RESET_REQUEST        = 0x08, /**< Request the receiver to perform device reset. */
} zigbee_benchmark_ctrl_t;

struct benchmark_address_context_t
{
    zb_uint16_t nwk_addr; /**< Remember only network address. EUI64 address is stored as device_id inside global device table. */
};

struct __packed benchmark_tx_power {
    int8_t power;
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

/**@brief Function for sending SET TX POWER to the remote peer.
 *
 * @param[in]           Network address of the remote peer.
 * @param[in]           Tx power required to set by remote peer.
 */
void zigbee_benchmark_set_tx_power_remote(zb_uint16_t peer_addr, zb_int8_t tx_power);

/**@brief Function for sending GET TX POWER to the remote peer.
 *
 * @param[in]           Network address of the remote peer.
 */
void zigbee_benchmark_get_tx_power_remote(zb_bufid_t bufid, zb_uint16_t peer_addr);

/**@brief Function for sending DEVICE RESET to the remote peer.
 *
 * @param[in]           Network address of the remote peer.
 */
void zigbee_benchmark_device_reset_remote(zb_bufid_t bufid, zb_uint16_t peer_addr);

/**@brief Function for sending OPEN NETWORK to the remote peer.
 *
 * @param[in]           Network address of the remote peer.
 */
void zigbee_benchmark_open_network_remote(zb_bufid_t bufid);

#endif /* BENCHMARK_ZIGBEE_COMMON_H__ */
