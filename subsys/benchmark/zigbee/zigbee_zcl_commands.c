/*$$$LICENCE_NORDIC_STANDARD<2022>$$$*/

#include "benchmark_api.h"
#include "benchmark_zigbee_common.h"
#include "zboss_api.h"
// #include "zigbee_cli.h"
// #include "zigbee_cli_utils.h"
#include <logging/log.h>

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

/**@brief ZCL Frame control field of Zigbee benchmark commands, that expects command execution result.
 */
#define ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD  ZB_ZCL_CONSTRUCT_FRAME_CONTROL(ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC, \
                                                                             ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,   \
                                                                             ZB_ZCL_FRAME_DIRECTION_TO_SRV,      \
                                                                             ZB_ZCL_ENABLE_DEFAULT_RESPONSE)

/**@brief ZCL Frame control field of Zigbee benchmark commands, that does not expect command execution result.
 */
#define ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD_NORESP  ZB_ZCL_CONSTRUCT_FRAME_CONTROL(ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC, \
                                                                                    ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,   \
                                                                                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,      \
                                                                                    ZB_ZCL_DISABLE_DEFAULT_RESPONSE)

/**@brief Custom benchmark cluster ID, used inside all control commands.
 */
#define BENCHMARK_CUSTOM_CLUSTER        0xBEC4


static zb_uint8_t m_seq_num = 0; /* ZCL sequence number for benchmark commands. */

/**@brief Forward declaration of function that sends local test results over Zigbee network. */
static zb_zcl_status_t zigbee_benchmark_peer_results_response_send(zb_bufid_t bufid, zb_uint16_t peer_addr, benchmark_result_t * p_results);
zb_uint8_t zb_cli_ep_handler(zb_bufid_t bufid);
static zb_zcl_status_t zigbee_benchmark_set_tx_power(struct benchmark_tx_power *requested_power);
static zb_zcl_status_t zigbee_benchmark_send_tx_power_response(zb_bufid_t bufid, zb_uint16_t remote_addr);

/**@bried Default handler for incoming benchmark command APS acknowledgments.
 *
 * @param[in] bufid  Reference to a ZBoss buffer containing APC ACK data,
 */
static void zigbee_benchmark_frame_error_handler(zb_bufid_t bufid)
{
    zb_zcl_command_send_status_t * p_cmd_status;

    if (bufid == 0)
    {
        return;
    }

    p_cmd_status = ZB_BUF_GET_PARAM(bufid, zb_zcl_command_send_status_t);
    LOG_DBG("Frame acknowledged. Status: %u\n", p_cmd_status->status);

    if (p_cmd_status->status != RET_OK)
    {
        /* No APS-ACK received. Abort benchmark test execution. */
        zigbee_benchmark_test_abort();
    }

    zb_buf_free(bufid);
}

/**@brief The Handler to 'intercept' every frame coming to the endpoint
 *
 * @param bufid    Reference to a ZBoss buffer
 */
zb_uint8_t zigbee_benchmark_ep_handler(zb_bufid_t bufid)
{
    zb_uint16_t           remote_short_addr;
    zb_zcl_parsed_hdr_t * p_cmd_info    = ZB_BUF_GET_PARAM(bufid, zb_zcl_parsed_hdr_t);
    zb_zcl_status_t       zb_zcl_status = ZB_ZCL_STATUS_SUCCESS;
    zb_bool_t             send_response = ZB_FALSE;

    if (p_cmd_info->cluster_id != BENCHMARK_CUSTOM_CLUSTER ||
        p_cmd_info->profile_id != ZB_AF_HA_PROFILE_ID)
    {
        return ZB_FALSE;
    }

    LOG_DBG("New benchmark frame received, bufid: %u\n", bufid);
    if (p_cmd_info->addr_data.common_data.source.addr_type != ZB_ZCL_ADDR_TYPE_SHORT)
    {
        return ZB_FALSE;
    }
    remote_short_addr = p_cmd_info->addr_data.common_data.source.u.short_addr;

    switch (p_cmd_info->cmd_id) {
        case TEST_START_REQUEST:
            LOG_DBG("Remote peer 0x%04x started benchmark test.", remote_short_addr);
            zb_zcl_status = zigbee_benchmark_test_start_slave();
            send_response = ZB_TRUE;
            break;
        case TEST_STOP_REQUEST:
            LOG_DBG("Remote peer 0x%04x stopped benchmark test.", remote_short_addr);
            zb_zcl_status = zigbee_benchmark_test_stop_slave();
            send_response = ZB_TRUE;
            break;
        case TEST_RESULTS_REQUEST:
            LOG_DBG("Remote peer 0x%04x asked for benchmark results.", remote_short_addr);
            zb_zcl_status = zigbee_benchmark_peer_results_response_send(bufid, remote_short_addr,
                                                                        zigbee_benchmark_local_result_get());
            if (zb_zcl_status != ZB_ZCL_STATUS_SUCCESS)
            {
                send_response = ZB_TRUE;
            }
            else
            {
                bufid = 0;
            }
            break;
        case TEST_RESULTS_RESPONSE:
            LOG_DBG("Remote peer 0x%04x sent benchmark results.", remote_short_addr);
            if (zb_buf_len(bufid) != sizeof(benchmark_result_t))
            {
                zigbee_benchmark_test_abort();
            }
            else
            {
                zigbee_benchmark_results_received((benchmark_result_t *)zb_buf_begin(bufid));
            }
            break;
        case TEST_RESET_REQUEST:
            LOG_DBG("Remote peer 0x%04x requested device reset.", remote_short_addr);
            ZB_SCHEDULE_APP_CALLBACK(zb_reset, 0);
            break;
        case TEST_SET_TX_POWER:
            if (bufid && (zb_buf_len(bufid) == sizeof(struct benchmark_tx_power))) {
                zb_zcl_status = zigbee_benchmark_set_tx_power((struct benchmark_tx_power*)zb_buf_begin(bufid));
            } else {
                zb_zcl_status = ZB_ZCL_STATUS_FAIL;
            }
            send_response = ZB_TRUE;
            break;
        case TEST_GET_TX_POWER:
            zb_zcl_status = zigbee_benchmark_send_tx_power_response(bufid, remote_short_addr);
            if (zb_zcl_status != ZB_ZCL_STATUS_SUCCESS)
            {
                send_response = ZB_TRUE;
            }
            else
            {
                bufid = 0;
            }
            break;
        case TEST_TX_POWER_RESPONSE:
            if (zb_buf_len(bufid) == sizeof(struct benchmark_tx_power)) {
                LOG_INF("Tx power of a remote peer (0x%04x) is %d",
                        remote_short_addr,
                        ((struct benchmark_tx_power*)zb_buf_begin(bufid))->power);
            }
            break;
        case TEST_OPEN_NETWORK_REQUEST:
            if (zb_get_network_role() == ZB_NWK_DEVICE_TYPE_COORDINATOR) {
                LOG_INF("Remote peer 0x%04x requested to open the network", remote_short_addr);
                zb_zcl_status = ZB_ZCL_STATUS_SUCCESS;

                if (bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING)) {
                    LOG_INF("Opened the network for joining");
                } else {
                    LOG_INF("Commissioning hasn't finished yet!");
                }
            } else {
                LOG_WRN("Remote peer 0x%04x requested to open the network, but "
                        "the device is not the network coordinator", remote_short_addr);
                zb_zcl_status = ZB_ZCL_STATUS_FAIL;
            }
            send_response = ZB_TRUE;
            break;
        case ZB_ZCL_CMD_DEFAULT_RESP:
        {
            zb_zcl_default_resp_payload_t * p_def_resp;
            p_def_resp = ZB_ZCL_READ_DEFAULT_RESP(bufid);
            LOG_DBG("Default Response received. Command: %u, Status: %u",
                    p_def_resp->command_id, p_def_resp->status);
            zigbee_benchmark_command_response_handler((zigbee_benchmark_ctrl_t)p_def_resp->command_id,
                                                      (zb_zcl_status_t)p_def_resp->status);
            break;
        }
        default:
            LOG_DBG("Unsupported benchmark command received, cmd_id %u", p_cmd_info->cmd_id);
            zb_zcl_status = ZB_ZCL_STATUS_UNSUP_MANUF_CLUST_CMD;
            send_response = ZB_TRUE;
            break;
    }

    if (bufid)
    {
        if (send_response)
        {
            zb_uint8_t cli_ep = zb_shell_get_endpoint();
            zb_uint8_t cmd_id = p_cmd_info->cmd_id;

            (void)zb_buf_reuse(bufid);
            LOG_DBG("Send (benchmark) default response. Command: %u, status: %u", p_cmd_info->cmd_id, zb_zcl_status);
            ZB_ZCL_SEND_DEFAULT_RESP(bufid, remote_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, cli_ep, cli_ep,
                                     ZB_AF_HA_PROFILE_ID, BENCHMARK_CUSTOM_CLUSTER, m_seq_num++, cmd_id, zb_zcl_status);
        }
        else
        {
            zb_buf_free(bufid);
        }
    }
    return ZB_TRUE;
}

zb_uint8_t zigbee_bm_ep_handler(zb_bufid_t bufid)
{
    LOG_DBG("ZIGBEE Benchmark EP handler");
    if (zigbee_benchmark_ep_handler(bufid))
    {
        return ZB_TRUE;
    }

    return zb_cli_ep_handler(bufid);
}

/**@brief Function to send benchmark control commands.
 *
 * @param[in] bufid      Reference number to ZBOSS memory buffer.
 * @param[in] peer_addr  Network address of the peer to control.
 * @param[in] ctrl_cmd   Control command Id.
 *
 * @retval RET_OK if command was successfully send, error code otherwise.
 */
static zb_ret_t zigbee_benchmark_ctrl_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr, zigbee_benchmark_ctrl_t ctrl_cmd)
{
    zb_uint8_t   cli_ep = zb_shell_get_endpoint();
    zb_ret_t     zb_err_code;
    zb_uint8_t * p_cmd_buf;
    zb_addr_u    remote_addr = {.addr_short = peer_addr};

    if (!bufid)
    {
        return RET_INVALID_PARAMETER_1;
    }

    p_cmd_buf = ZB_ZCL_START_PACKET(bufid);

    // Add frame control field based on request type.
    switch (ctrl_cmd)
    {
        case TEST_RESULTS_REQUEST:
            *(p_cmd_buf++) = ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD_NORESP;
            break;

        default:
            *(p_cmd_buf++) = ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD;
            break;
    }

    *(p_cmd_buf++) = m_seq_num; /* Sequence Number Field */
    *(p_cmd_buf++) = ctrl_cmd;  /* Command ID field */
    m_seq_num++;

    LOG_DBG("Send benchmark control command. ID: %u\n", ctrl_cmd);
    zb_err_code = zb_zcl_finish_and_send_packet(bufid, p_cmd_buf, &remote_addr,
                                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, cli_ep, cli_ep,
                                                ZB_AF_HA_PROFILE_ID, BENCHMARK_CUSTOM_CLUSTER,
                                                zigbee_benchmark_frame_error_handler);
    return zb_err_code;
}

/**@brief Function to send local results to the other peer.
 *
 * @param[in] bufid      Reference number to ZBOSS memory buffer.
 * @param[in] peer_addr  Network address of the peer that asked for test results.
 * @param[in] p_result   Pointer to the benchmark test results, that will be copied to the Zigbee command frame.
 *
 * @retval ZB_ZCL_STATUS_SUCCESS if results were successfully send.
 * @retval ZB_ZCL_STATUS_FAIL    if failed to send results.
 */
static zb_zcl_status_t zigbee_benchmark_peer_results_response_send(zb_bufid_t bufid, zb_uint16_t peer_addr, benchmark_result_t * p_result)
{
    zb_uint8_t   cli_ep = zb_shell_get_endpoint();
    zb_ret_t     zb_err_code;
    zb_uint8_t  *p_cmd_buf;
    zb_addr_u    remote_addr = {.addr_short = peer_addr};

    if (!bufid)
    {
        return ZB_ZCL_STATUS_FAIL;
    }

    p_cmd_buf = ZB_ZCL_START_PACKET(bufid);
    *(p_cmd_buf++) = ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD_NORESP;
    *(p_cmd_buf++) = m_seq_num; /* Sequence Number Field */
    *(p_cmd_buf++) = TEST_RESULTS_RESPONSE;
    memcpy(p_cmd_buf, p_result, sizeof(benchmark_result_t));
    p_cmd_buf += sizeof(benchmark_result_t);
    m_seq_num++;

    LOG_DBG("Send benchmark results from the remote peer to 0x%04x", peer_addr);
    zb_err_code = zb_zcl_finish_and_send_packet(bufid, p_cmd_buf, &remote_addr,
                                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, cli_ep, cli_ep,
                                                ZB_AF_HA_PROFILE_ID, BENCHMARK_CUSTOM_CLUSTER,
                                                zigbee_benchmark_frame_error_handler);
    return (zb_err_code == RET_OK ? ZB_ZCL_STATUS_SUCCESS : ZB_ZCL_STATUS_FAIL);
}


void zigbee_benchmark_peer_results_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr)
{
    zb_ret_t err_code = zigbee_benchmark_ctrl_request_send(bufid, peer_addr, TEST_RESULTS_REQUEST);
    if (err_code != RET_OK)
    {
        LOG_DBG("Sending results request (ID: %u) to the remote peer (nwk_addr: 0x%04x) failed with error: %d",
               TEST_RESULTS_REQUEST, peer_addr, err_code);
        zigbee_benchmark_test_abort();
    }
}

void zigbee_benchmark_get_tx_power_remote(zb_bufid_t bufid, zb_uint16_t peer_addr)
{
    zb_ret_t err_code = zigbee_benchmark_ctrl_request_send(bufid, peer_addr, TEST_GET_TX_POWER);
    if (err_code != RET_OK)
    {
        LOG_ERR("Sending GET TX POWER request (ID: %u) to the remote peer (nwk_addr: 0x%04x) "
                "failed with error: %d",
                TEST_GET_TX_POWER, peer_addr, err_code);
    }
}

void zigbee_benchmark_device_reset_remote(zb_bufid_t bufid, zb_uint16_t peer_addr)
{
    zb_ret_t err_code = zigbee_benchmark_ctrl_request_send(bufid, peer_addr, TEST_RESET_REQUEST);
    if (err_code != RET_OK)
    {
        LOG_ERR("Sending RESET DEVICE request (ID: %u) to the remote peer (nwk_addr: 0x%04x) "
                "failed with error: %d",
                TEST_RESET_REQUEST, peer_addr, err_code);
    }
}

void zigbee_benchmark_open_network_remote(zb_bufid_t bufid)
{
    zb_ret_t err_code = zigbee_benchmark_ctrl_request_send(bufid, 0x0000, TEST_OPEN_NETWORK_REQUEST);
    if (err_code != RET_OK)
    {
        LOG_ERR("Sending RESET DEVICE request (ID: %u) to the remote peer (nwk_addr: 0x%04x) "
                "failed with error: %d",
                TEST_OPEN_NETWORK_REQUEST, 0x0000, err_code);
    }
}

void zigbee_benchmark_set_tx_power_remote(zb_uint16_t peer_addr, zb_int8_t tx_power_val)
{
    zb_uint8_t   cli_ep = zb_shell_get_endpoint();
    zb_ret_t     zb_err_code;
    zb_uint8_t  *p_cmd_buf;
    zb_addr_u    remote_addr = {.addr_short = peer_addr};
    struct benchmark_tx_power tx_power = { .power = tx_power_val };
    zb_bufid_t bufid = zb_buf_get_out();

    if (!bufid)
    {
        LOG_ERR("Failed to allocate ZBOSS buffer");
        return;
    }

    p_cmd_buf = ZB_ZCL_START_PACKET(bufid);
    *(p_cmd_buf++) = ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD_NORESP;
    *(p_cmd_buf++) = m_seq_num; /* Sequence Number Field */
    *(p_cmd_buf++) = TEST_SET_TX_POWER;
    memcpy(p_cmd_buf, &tx_power, sizeof(struct benchmark_tx_power));
    p_cmd_buf += sizeof(struct benchmark_tx_power);
    m_seq_num++;

    LOG_DBG("Send benchmark SET TX POWER from the remote peer to 0x%04x", peer_addr);
    zb_err_code = zb_zcl_finish_and_send_packet(bufid, p_cmd_buf, &remote_addr,
                                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, cli_ep, cli_ep,
                                                ZB_AF_HA_PROFILE_ID, BENCHMARK_CUSTOM_CLUSTER,
                                                zigbee_benchmark_frame_error_handler);

    if (zb_err_code != RET_OK) {
        LOG_ERR("Sending benchmark command SET TX POWER failed");
    }
}

void zigbee_benchmark_peer_start_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr)
{
    zb_ret_t err_code = zigbee_benchmark_ctrl_request_send(bufid, peer_addr, TEST_START_REQUEST);
    if (err_code != RET_OK)
    {
        LOG_DBG("Sending test start request (ID: %u) to the remote peer (nwk_addr: 0x%04x) failed with error: %d",
                TEST_START_REQUEST, peer_addr, err_code);
        zigbee_benchmark_test_abort();
    }
}

void zigbee_benchmark_peer_stop_request_send(zb_bufid_t bufid, zb_uint16_t peer_addr)
{
    zb_ret_t err_code = zigbee_benchmark_ctrl_request_send(bufid, peer_addr, TEST_STOP_REQUEST);
    if (err_code != RET_OK)
    {
        LOG_DBG("Sending test stop request (ID: %u) to the remote peer (nwk_addr: 0x%04x) failed with error: %d",
                TEST_STOP_REQUEST, peer_addr, err_code);
        zigbee_benchmark_test_abort();
    }
}

static zb_zcl_status_t zigbee_benchmark_set_tx_power(struct benchmark_tx_power *requested_power)
{
    struct benchmark_tx_power actual_power;
    zb_zcl_status_t zcl_status = ZB_ZCL_STATUS_FAIL;

    zb_mac_set_tx_power(requested_power->power);
    actual_power.power = zb_mac_get_tx_power();

    if (actual_power.power == requested_power->power) {
        LOG_INF("TX Power set to %d", actual_power.power);
        zcl_status = ZB_ZCL_STATUS_SUCCESS;
    } else {
        LOG_WRN("Setting TX Power failed");
        zcl_status = ZB_ZCL_STATUS_FAIL;
    }

    return zcl_status;
}

static zb_zcl_status_t zigbee_benchmark_send_tx_power_response(zb_bufid_t bufid, zb_uint16_t peer_addr)
{
    zb_uint8_t   cli_ep = zb_shell_get_endpoint();
    zb_addr_u    remote_addr = {.addr_short = peer_addr};
    struct benchmark_tx_power power;
    zb_ret_t     zb_err_code;
    zb_uint8_t  *p_cmd_buf;

    if (!bufid)
    {
        return ZB_ZCL_STATUS_FAIL;
    }

    power.power = zb_mac_get_tx_power();

    p_cmd_buf = ZB_ZCL_START_PACKET(bufid);
    *(p_cmd_buf++) = ZIGBEE_BENCHMARK_FRAME_CONTROL_FIELD_NORESP;
    *(p_cmd_buf++) = m_seq_num; /* Sequence Number Field */
    *(p_cmd_buf++) = TEST_TX_POWER_RESPONSE;
    memcpy(p_cmd_buf, &power, sizeof(struct benchmark_tx_power));
    p_cmd_buf += sizeof(struct benchmark_tx_power);
    m_seq_num++;

    LOG_DBG("Send tx power from the remote peer to 0x%04x", peer_addr);
    zb_err_code = zb_zcl_finish_and_send_packet(bufid, p_cmd_buf, &remote_addr,
                                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, cli_ep, cli_ep,
                                                ZB_AF_HA_PROFILE_ID, BENCHMARK_CUSTOM_CLUSTER,
                                                zigbee_benchmark_frame_error_handler);
    return (zb_err_code == RET_OK ? ZB_ZCL_STATUS_SUCCESS : ZB_ZCL_STATUS_FAIL);
}
