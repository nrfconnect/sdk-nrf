/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <string.h>

#include <zephyr.h>
#include <zboss_api.h>
// #include <zigbee_cli.h>
#include <logging/log.h>

#include "benchmark_api.h"
#include "benchmark_zigbee_common.h"
#include "zigbee_shell_ping_types.h"
#include "zigbee_shell_zcl_types.h"
#include "zigbee_shell_ctx_mgr.h"
#include "zigbee/zigbee_app_utils.h"
#include "zigbee_benchmark_internal.h"

LOG_MODULE_REGISTER(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

#define BENCHMARK_THREAD_STACK_SIZE 1024
#define BENCHMARK_THREAD_PRIO       7
#define BENCHMARK_THREAD_OPTS       0
#define BENCHMARK_THREAD_NAME       "benchmark"

extern int cmd_zb_ping_generic(const struct shell *shell, struct ping_req *ping_req, struct zcl_packet_info *pkt_info);
static void zigbee_benchmark_send_ping_req(void);

typedef union
{
    zb_ieee_addr_t arr_addr;
    uint64_t       uint_addr;
} eui64_map_addr_t;

static void discovery_timeout(struct k_timer *timer);

K_THREAD_STACK_DEFINE(benchmark_thread_stack, BENCHMARK_THREAD_STACK_SIZE);

K_TIMER_DEFINE(discovery_timer, discovery_timeout, NULL);

static zb_time_t                     m_start_time;
static benchmark_evt_t               m_benchmark_evt;
static benchmark_result_t            m_local_result;
static benchmark_result_t            m_remote_result;
static benchmark_callback_t          mp_callback;
static benchmark_peer_db_t           m_peer_information;
static benchmark_address_context_t   m_peer_addresses[BENCHMARK_MAX_PEER_NUMBER];
static benchmark_configuration_t    *mp_test_configuration;
static benchmark_test_state_t        m_state = TEST_IDLE;
static struct k_thread               benchmark_thread;

extern struct shell const *p_shell;

static benchmark_status_t m_test_status =
{
    .test_in_progress   = false,
    .reset_counters     = false,
    .waiting_for_ack    = 0,
    .packets_left_count = 0,
    .acks_lost          = 0,
    .frame_number       = 0,
    .latency            =
    {
        .min = UINT32_MAX,
        .max = UINT32_MAX
    },
};

/**@brief Function that triggers calculation of the test duration. */
static void benchmark_test_duration_calculate(void)
{
    m_local_result.duration = timer_ticks_from_uptime() - m_start_time;
}

/**@brief Function that reads current MAC-level statistics from the radio.
  *
  * @param[out] p_mac_tx_counters  Pointer to the structure holding MAC-level TX statistics.
  *
  * @note Returned values are reset to zero only after system reboot.
  */
static void mac_counters_read(benchmark_mac_counters_t * p_mac_tx_counters)
{
#if defined ZIGBEE_NRF_RADIO_STATISTICS
    zigbee_nrf_radio_stats_t * p_stats = zigbee_get_nrf_radio_stats();

    // LOG_DBG("Radio RX statistics:");
    // LOG_DBG("\trx_successful: %ld", p_stats->rx_successful);
    // LOG_DBG("\trx_err_none: %ld", p_stats->rx_err_none);
    // LOG_DBG("\trx_err_invalid_frame: %ld", p_stats->rx_err_invalid_frame);
    // LOG_DBG("\trx_err_invalid_fcs: %ld", p_stats->rx_err_invalid_fcs);
    // LOG_DBG("\trx_err_invalid_dest_addr: %ld", p_stats->rx_err_invalid_dest_addr);
    // LOG_DBG("\trx_err_runtime: %ld", p_stats->rx_err_runtime);
    // LOG_DBG("\trx_err_timeslot_ended: %ld", p_stats->rx_err_timeslot_ended);
    // LOG_DBG("\trx_err_aborted: %ld\r\n", p_stats->rx_err_aborted);

    // LOG_DBG("Radio TX statistics:");
    // LOG_DBG("\ttx_successful: %d", p_stats->tx_successful);
    // LOG_DBG("\ttx_err_none: %d", p_stats->tx_err_none);
    // LOG_DBG("\ttx_err_busy_channel: %d", p_stats->tx_err_busy_channel);
    // LOG_DBG("\ttx_err_invalid_ack: %d", p_stats->tx_err_invalid_ack);
    // LOG_DBG("\ttx_err_no_mem: %d", p_stats->tx_err_no_mem);
    // LOG_DBG("\ttx_err_timeslot_ended: %d", p_stats->tx_err_timeslot_ended);
    // LOG_DBG("\ttx_err_no_ack: %d", p_stats->tx_err_no_ack);
    // LOG_DBG("\ttx_err_aborted: %d", p_stats->tx_err_aborted);
    // LOG_DBG("\ttx_err_timeslot_denied: %d", p_stats->tx_err_timeslot_denied);

    p_mac_tx_counters->error = p_stats->tx_err_busy_channel + p_stats->tx_err_no_mem
                               + p_stats->tx_err_invalid_ack + p_stats->tx_err_no_ack
                               + p_stats->tx_err_aborted + p_stats->tx_err_timeslot_ended + p_stats->tx_err_timeslot_denied;
    p_mac_tx_counters->total = p_stats->tx_successful + p_mac_tx_counters->error;

#else /* ZIGBEE_NRF_RADIO_STATISTICS */

    p_mac_tx_counters->error = 0xFFFFFFFFUL;
    p_mac_tx_counters->total = 0xFFFFFFFFUL;

#endif /* ZIGBEE_NRF_RADIO_STATISTICS */
}

/**@brief Function that clears the radio MAC-level TX statistics. */
static void mac_counters_clear(void)
{
    mac_counters_read(&m_local_result.mac_tx_counters);
}

/**@brief Function that triggers calculation of the radio MAC-level TX statistics. */
static void mac_counters_calculate(void)
{
    benchmark_mac_counters_t current_mac_tx_counters;

    // Read current radio TX statistics.
    mac_counters_read(&current_mac_tx_counters);

    // Subtract from statistics from the beginning of the test.
    m_local_result.mac_tx_counters.total = current_mac_tx_counters.total - m_local_result.mac_tx_counters.total;
    m_local_result.mac_tx_counters.error = current_mac_tx_counters.error - m_local_result.mac_tx_counters.error;
}

/**@brief Function that resets all test counters. */
static void result_clear(void)
{
    memset(&m_local_result, 0, sizeof(m_local_result));
    memset(&m_remote_result, 0, sizeof(m_remote_result));
    cpu_utilization_clear();
    benchmark_clear_latency(&m_test_status.latency);
    mac_counters_clear();

    m_local_result.rx_counters.rx_total = BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED;
    m_local_result.rx_counters.rx_error = BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED;
}

/**@brief Callback for discovery timeout event.
 *
 * @param[in] p_context Unused parameter.
 */
static void discovery_timeout(struct k_timer *timer)
{
    m_benchmark_evt.evt                        = BENCHMARK_DISCOVERY_COMPLETED;
    m_benchmark_evt.context.p_peer_information = &m_peer_information;

    LOG_DBG("Discovery timeout");

    if (mp_callback)
    {
        mp_callback(&m_benchmark_evt);
    }
}

/**@brief A callback called on EUI64 address response.
 *
 * @param[in] bufid Reference number to ZBOSS memory buffer.
 */
static void zb_resolve_ieee_addr_cb(zb_bufid_t bufid)
{
    zb_zdo_ieee_addr_resp_t * p_resp = (zb_zdo_ieee_addr_resp_t *)zb_buf_begin(bufid);

    if (p_resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_address_ieee_ref_t addr_ref;
        eui64_map_addr_t      ieee_addr;
        zb_uint16_t           nwk_addr;
        uint_fast8_t          i;

        ZB_LETOH64(ieee_addr.arr_addr, p_resp->ieee_addr_remote_dev);
        ZB_LETOH16(&nwk_addr, &(p_resp->nwk_addr_remote_dev));
        zb_address_update(ieee_addr.arr_addr, nwk_addr, ZB_TRUE, &addr_ref);
        // LOG_DBG("Received EUI64 address for device 0x%04x -> %ld.", nwk_addr, ieee_addr.uint_addr);

        for (i = 0; i < m_peer_information.peer_count; i++)
        {
            if (m_peer_information.peer_table[i].p_address->nwk_addr == nwk_addr)
            {
                m_peer_information.peer_table[i].device_id = ieee_addr.uint_addr;
                // LOG_DBG("The device with address 0x%04x is updated.", nwk_addr);
                break;
            }
        }
    }
    else
    {
        // LOG_WARNING("Unable to resolve EUI64 source address. Status: %d\r\n",
                        // p_resp->status);
    }

    zb_buf_free(bufid);
}

/**@brief Resolve EUI64 by sending IEEE address request.
 *
 * @param[in] bufid     Reference number to ZBOSS memory buffer.
 * @param[in] nwk_addr  Network address to be resolved.
 */
static void zb_resolve_ieee_addr(zb_bufid_t bufid, zb_uint16_t nwk_addr)
{
    zb_zdo_ieee_addr_req_param_t * p_req = NULL;
    zb_uint8_t                     tsn = 0;

    // Create new IEEE address request and fill with default values.
    p_req = ZB_BUF_GET_PARAM(bufid, zb_zdo_ieee_addr_req_param_t);
    p_req->start_index  = 0;
    p_req->request_type = 0;
    p_req->nwk_addr     = nwk_addr;
    p_req->dst_addr     = p_req->nwk_addr;

    tsn = zb_zdo_ieee_addr_req(bufid, zb_resolve_ieee_addr_cb);
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
        // LOG_WARNING("Failed to send IEEE address request for address: 0x%04x", nwk_addr);
        zb_buf_free(bufid);
    }
}

/**@brief A callback called on match descriptor response.
 *
 * @param[in] bufid a reference number to ZBOSS memory buffer
 */
static void zb_find_peers_cb(zb_bufid_t bufid)
{
    zb_zdo_match_desc_resp_t   * p_resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(bufid);
    zb_apsde_data_indication_t * p_ind  = ZB_BUF_GET_PARAM(bufid, zb_apsde_data_indication_t);

    if (p_resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t * p_match_ep = (zb_uint8_t *)(p_resp + 1);
        zb_uint8_t   match_len  = p_resp->match_len;

        while (match_len > 0)
        {
            /* Match EP list follows right after response header */
            if (*p_match_ep == zb_shell_get_endpoint())
            {
                uint16_t peer_number = m_peer_information.peer_count;

                // LOG_INFO("Peer found: %0hx", p_ind->src_addr);

                if (peer_number < BENCHMARK_MAX_PEER_NUMBER)
                {
                    m_peer_information.peer_table[peer_number].device_id = 0;
                    m_peer_information.peer_table[peer_number].p_address = &m_peer_addresses[peer_number];
                    m_peer_information.peer_table[peer_number].p_address->nwk_addr = p_ind->src_addr;
                    m_peer_information.peer_count++;

                    // Try to resolve EUI64 address based on NWK address (best effort).
                    zb_resolve_ieee_addr(bufid, p_ind->src_addr);
                    bufid = 0;
                }
                else
                {
                    // LOG_INFO("Can't add peer to the list, list full.");
                }
            }

            p_match_ep += 1;
            match_len -= 1;
        }
    }

    if (bufid)
    {
        zb_buf_free(bufid);
    }
}

/**@brief Function that constructs and sends discovery request. */
static zb_ret_t zigbee_benchmark_peer_discovery_request_send(void)
{
    zb_zdo_match_desc_param_t * p_req;
    zb_bufid_t                  bufid;

    bufid = zb_buf_get_out();
    if (!bufid)
    {
        LOG_ERR("Failed to execute command (buffer allocation failed).");
        return RET_ERROR;
    }

    /* Initialize pointers inside buffer and reserve space for zb_zdo_match_desc_param_t request */
    p_req = zb_buf_initial_alloc(bufid, sizeof(zb_zdo_match_desc_param_t));

    p_req->nwk_addr         = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE; // Send to all non-sleepy devices
    p_req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE; // Get responses from all non-sleepy devices
    p_req->profile_id       = ZB_AF_HA_PROFILE_ID;              // Look for Home Automation profile clusters

    /* We are searching for 1 input cluster: Basic */
    p_req->num_in_clusters  = 1;
    p_req->num_out_clusters = 0;
    p_req->cluster_list[0]  = ZB_ZCL_CLUSTER_ID_BASIC;

    uint8_t tsn = zb_zdo_match_desc_req(bufid, zb_find_peers_cb);
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
        zb_buf_free(bufid);
        return RET_ERROR;
    }

    return RET_OK;
}

static void schedule_next_frame(void)
{
    if (m_test_status.packets_left_count > 0)
    {
        // LOG_DBG("Test frame sent, prepare next frame.");
        m_state = TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER;
        zigbee_benchmark_send_ping_req();
    }
    else
    {
        LOG_INF("Test frame sent, test finished.");
        m_state = TEST_FINISHED;
    }
}

/**@brief  Ping event handler. Updates RX counters and changes state to send next request.
 *
 * @param[in] evt_type  Type of received  ping acknowledgment
 * @param[in] delay_us  Time, in microseconds, between ping request and the event.
 * @param[in] p_request Pointer to the ongoing ping request context structure.
 */
static void benchmark_ping_evt_handler(enum ping_time_evt evt, zb_uint32_t delay_us,
                                       struct ctx_entry *entry)
{
    struct ping_req *p_request = &entry->zcl_data.ping_req;

    LOG_DBG("Benchmark ping evt handler!! state %u", evt);

    switch (evt)
    {
        case PING_EVT_FRAME_SENT:
            if (m_state == TEST_IN_PROGRESS_FRAME_SENDING)
            {
                m_state = TEST_IN_PROGRESS_FRAME_SENT;
                schedule_next_frame();
            }
            break;

        case PING_EVT_ACK_RECEIVED:
            if (m_state == TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK)
            {
                m_test_status.waiting_for_ack = 0;
                m_state = TEST_IN_PROGRESS_FRAME_SENT;
                benchmark_update_latency(&m_test_status.latency, delay_us / 2);
                // LOG_DBG("Transmission time: %u us", delay_us / 2);
                schedule_next_frame();
            }
            break;

        case PING_EVT_ECHO_RECEIVED:
            if (m_state == TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO)
            {
                m_test_status.waiting_for_ack = 0;
                m_state = TEST_IN_PROGRESS_FRAME_SENT;
                benchmark_update_latency(&m_test_status.latency, delay_us / 2);
                // LOG_DBG("Transmission time: %u us", delay_us / 2);
                schedule_next_frame();
            }
            break;

        case PING_EVT_FRAME_TIMEOUT:
            if ((m_state == TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK) ||
                (m_state == TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO))
            {
                m_test_status.waiting_for_ack = 0;
                m_test_status.acks_lost++;
                m_state = TEST_IN_PROGRESS_FRAME_SENT;
                // LOG_DBG("Transmission timed out.");
                schedule_next_frame();
            }
            break;

        case PING_EVT_FRAME_SCHEDULED:
            if (m_test_status.packets_left_count)
            {
                m_test_status.packets_left_count--;
            }

            if (m_state == TEST_IN_PROGRESS_FRAME_SENDING)
            {
                if (mp_test_configuration)
                {
                    if (mp_test_configuration->mode != BENCHMARK_MODE_UNIDIRECTIONAL)
                    {
                        m_state = TEST_IN_PROGRESS_FRAME_SENT;
                        schedule_next_frame();
                    }
                }
                else
                {
                    m_state = TEST_IN_PROGRESS_FRAME_SENT;
                    schedule_next_frame();
                }
            }
            else
            {
                m_test_status.waiting_for_ack = p_request->ping_seq;
                m_test_status.frame_number    = p_request->ping_seq;
            }
            break;

        case PING_EVT_ERROR:
            if ((m_state == TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK) ||
                (m_state == TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO) ||
                (m_state == TEST_IN_PROGRESS_FRAME_SENDING))
            {
                m_state = TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER;
            }
            break;

        case PING_EVT_REQUEST_RECEIVED:
            if (m_test_status.reset_counters)
            {
                // First packet in slave mode received.
                cpu_utilization_clear();
                // Reset MAC-level TX counters in order to ignore initial control message response.
                mac_counters_clear();
                m_test_status.reset_counters = false;
            }

            m_local_result.rx_counters.bytes_received += p_request->count;
            m_local_result.rx_counters.packets_received ++;

            break;
        default:
            break;
    }
}

/**@brief Function that constructs and sends ping request. */
static void zigbee_benchmark_send_ping_req(void)
{
    struct ping_req ping_req;
    struct zcl_packet_info pkt_info;

    if (m_test_status.packets_left_count == 0)
    {
        return;
    }

    if (m_state != TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER)
    {
        return;
    }

    pkt_info.dst_addr.addr_short =
        m_peer_information.peer_table[m_peer_information.selected_peer].p_address->nwk_addr;
    pkt_info.dst_addr_mode = ADDR_SHORT;
    ping_req.count = mp_test_configuration->length;
    ping_req.timeout_ms = mp_test_configuration->ack_timeout;

    switch (mp_test_configuration->mode)
    {
        case BENCHMARK_MODE_UNIDIRECTIONAL:
            ping_req.request_ack  = 0;
            ping_req.request_echo = 0;
            m_state = TEST_IN_PROGRESS_FRAME_SENDING;
            break;

        case BENCHMARK_MODE_ECHO:
            ping_req.request_ack = 0;
            ping_req.request_echo = 1;
            m_state = TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO;
            break;

        case BENCHMARK_MODE_ACK:
            ping_req.request_ack  = 1;
            ping_req.request_echo = 0;
            m_state = TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK;
            break;

        default:
            LOG_WRN("Unsupported test mode - fall back to the default mode.");
            mp_test_configuration->mode = BENCHMARK_MODE_ECHO;
            ping_req.request_ack     = 0;
            ping_req.request_echo    = 1;
            m_state = TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO;
            break;
    }

    if (0 != cmd_zb_ping_generic(p_shell, &ping_req, &pkt_info))
    {
        LOG_ERR("Error occured while sending ping request");
        m_state = TEST_ERROR;
    }
}

/**@brief Function that starts benchmark test in master mode. */
static void zigbee_benchmark_test_start_master(void)
{
    if (m_state != TEST_IDLE)
    {
        LOG_WRN("Stop current test in order to start different suite.");
        return;
    }

    result_clear();
    // Ignore APS ACK transmission for the TEST_START_REQUEST command.
    m_local_result.mac_tx_counters.total++;

    cpu_utilization_start();
    m_test_status.test_in_progress = true;
    m_start_time                   = timer_ticks_from_uptime();

    m_state = TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER;
    zigbee_benchmark_send_ping_req();

    if (mp_callback)
    {
        m_benchmark_evt.evt           = BENCHMARK_TEST_STARTED;
        m_benchmark_evt.context.error = RET_OK;
        mp_callback(&m_benchmark_evt);
    }

    LOG_INF("Start benchmark on the local peer.");
}

/**@brief Function that stops benchmark test in master mode. */
static void zigbee_benchmark_test_stop_master(void)
{
    if (m_state != TEST_IDLE)
    {
        // LOG_ERR("Requested to get remote results, but the device is in an incorrect state.");
        return;
    }

    m_local_result.cpu_utilization = cpu_utilization_get();
    m_benchmark_evt.evt            = BENCHMARK_TEST_STOPPED;
    m_benchmark_evt.context.error  = benchmark_peer_results_request_send();

    if (mp_callback)
    {
        mp_callback(&m_benchmark_evt);
    }
}

/**@brief Function that starts benchmark test in slave mode. */
zb_zcl_status_t zigbee_benchmark_test_start_slave(void)
{
    if (m_state != TEST_IDLE)
    {
        // LOG_ERR("Stop current test in order to start different suite.");
        return ZB_ZCL_STATUS_FAIL;
    }

    m_state = TEST_IN_PROGRESS_WAITING_FOR_STOP_CMD;

    memset(&m_benchmark_evt, 0, sizeof(benchmark_evt_t));
    m_test_status.acks_lost = 0;
    m_start_time            = timer_ticks_from_uptime();

    result_clear();
    cpu_utilization_start();
    m_test_status.test_in_progress = true;
    m_test_status.reset_counters   = true;

    // Generate start event. That way CLI will be suppressed for the whole remote benchmark test.
    if (mp_callback)
    {
        m_benchmark_evt.evt           = BENCHMARK_TEST_STARTED;
        m_benchmark_evt.context.error = RET_OK;
        mp_callback(&m_benchmark_evt);
    }

    LOG_INF("Start benchmark on the remote peer.");
    return ZB_ZCL_STATUS_SUCCESS;
}

/**@brief Function that stops benchmark test in slave mode. */
zb_zcl_status_t zigbee_benchmark_test_stop_slave(void)
{
    // Check if any slave-test have been started.
    if (m_state != TEST_IN_PROGRESS_WAITING_FOR_STOP_CMD)
    {
        return ZB_ZCL_STATUS_FAIL;
    }

    mac_counters_calculate();
    m_local_result.cpu_utilization = cpu_utilization_get();
    cpu_utilization_stop();
    benchmark_test_duration_calculate();
    m_state = TEST_IDLE;
    m_test_status.test_in_progress = false;

    // Generate event in order to unlock CLI suppression on the slave board.
    if (mp_callback)
    {
        m_benchmark_evt.evt                             = BENCHMARK_TEST_COMPLETED;
        m_benchmark_evt.context.results.p_remote_result = NULL;
        m_benchmark_evt.context.results.p_local_result  = &m_local_result;
        m_benchmark_evt.context.results.p_local_status  = &m_test_status;
        mp_callback(&m_benchmark_evt);
    }

    // LOG_INFO("Benchmark finished on the remote peer.");
    return ZB_ZCL_STATUS_SUCCESS;
}

/**@brief Function that is called upon reception of remote test results. */
void zigbee_benchmark_results_received(benchmark_result_t * p_result)
{
    LOG_INF("Benchmark results received from the remote peer.");
    memcpy(&m_remote_result, p_result, sizeof(benchmark_result_t));

    m_benchmark_evt.evt                             = BENCHMARK_TEST_COMPLETED;
    m_benchmark_evt.context.results.p_remote_result = &m_remote_result;
    m_benchmark_evt.context.results.p_local_result  = &m_local_result;
    m_benchmark_evt.context.results.p_local_status  = &m_test_status;

    if (mp_callback)
    {
        mp_callback(&m_benchmark_evt);
    }
}

/**@brief Function that is called upon reception of benchmark control command response. */
void zigbee_benchmark_command_response_handler(zigbee_benchmark_ctrl_t cmd_id, zb_zcl_status_t status)
{
    if (status != ZB_ZCL_STATUS_SUCCESS)
    {
        m_benchmark_evt.context.error = status;

        switch (cmd_id)
        {
            case TEST_START_REQUEST:
                LOG_INF("Remote peer failed to start benchmark execution. Error: %d", status);
                m_benchmark_evt.evt = BENCHMARK_TEST_STARTED;
                break;

            case TEST_STOP_REQUEST:
                LOG_INF("Remote peer failed to stop benchmark execution. Error: %d", status);
                m_benchmark_evt.evt = BENCHMARK_TEST_STOPPED;
                break;

            case TEST_RESULTS_REQUEST:
                LOG_INF("Remote peer failed to send benchmark results. Error: %d", status);
                m_benchmark_evt.evt = BENCHMARK_TEST_COMPLETED;
                break;

            default:
                LOG_INF("Unsupported remote benchmark command response received. Command: %d", cmd_id);
                m_benchmark_evt.context.error = ZB_ZCL_STATUS_SUCCESS;
                break;
        }

        if ((mp_callback) && (m_benchmark_evt.context.error != ZB_ZCL_STATUS_SUCCESS))
        {
            mp_callback(&m_benchmark_evt);
        }

        zigbee_benchmark_test_abort();
        return;
    }

    switch (cmd_id)
    {
        case TEST_START_REQUEST:
            LOG_INF("Remote peer successfully started benchmark execution.");
            zigbee_benchmark_test_start_master();
            break;

        case TEST_STOP_REQUEST:
            LOG_INF("Remote peer successfully finished benchmark execution.");
            zigbee_benchmark_test_stop_master();
            break;

        default:
            LOG_INF("Unsupported remote benchmark command response received: %d", cmd_id);
            break;
    }
}

/**@brief Aborts current benchmark test execution. */
void zigbee_benchmark_test_abort(void)
{
    LOG_WRN("Abort benchmark execution.");
    benchmark_test_stop();
}

static void benchmark_thread_loop(void *p1, void *p2, void *p3)
{
    while (true)
    {
        benchmark_process();
        k_usleep(10);
    }
}

void benchmark_init(void)
{
    mp_test_configuration = NULL;

    zb_ping_set_ping_indication_cb(benchmark_ping_evt_handler);
    zb_ping_set_ping_event_cb(benchmark_ping_evt_handler);

    k_thread_create(&benchmark_thread, benchmark_thread_stack,
                    BENCHMARK_THREAD_STACK_SIZE, benchmark_thread_loop,
                    NULL, NULL, NULL, BENCHMARK_THREAD_PRIO,
                    BENCHMARK_THREAD_OPTS, K_NO_WAIT);

    k_thread_name_set((k_tid_t)&benchmark_thread, BENCHMARK_THREAD_NAME);

    LOG_INF("Benchmark component has been initialized");
}

uint32_t benchmark_test_init(benchmark_configuration_t * p_configuration, benchmark_callback_t callback)
{
    if (m_state != TEST_IDLE)
    {
        LOG_WRN("Stop current test in order to modify test settings.");
        return (uint32_t)RET_ERROR;
    }

    if ((p_configuration == NULL) || (callback == NULL))
    {
        LOG_WRN("Both configuration and callback have to be passed.");
        return (uint32_t)RET_ERROR;
    }

    if (callback == NULL)
    {
        LOG_WRN("Event callback have to be passed.");
        return (uint32_t)RET_ERROR;
    }

    mp_callback                      = callback;
    mp_test_configuration            = p_configuration;
    m_test_status.packets_left_count = p_configuration->count;
    m_test_status.waiting_for_ack    = 0;
    m_test_status.frame_number       = 0;

    return (uint32_t)RET_OK;
}

uint32_t benchmark_test_start(void)
{
    zb_uint16_t peer_addr;

    if (m_state != TEST_IDLE)
    {
        LOG_WRN("Stop current test in order to start different suite.");
        return (uint32_t)RET_ERROR;
    }

    if (mp_test_configuration == NULL)
    {
        LOG_WRN("Provide test configuration before starting a test suite.");
        return (uint32_t)RET_ERROR;
    }

    memset(&m_benchmark_evt, 0, sizeof(benchmark_evt_t));
    m_test_status.acks_lost      = 0;
    m_test_status.reset_counters = false;

    LOG_INF("Sending start request to the remote peer.");
    peer_addr = m_peer_information.peer_table[m_peer_information.selected_peer].p_address->nwk_addr;
    return (uint32_t)zb_buf_get_out_delayed_ext(zigbee_benchmark_peer_start_request_send, peer_addr, 0);
}

uint32_t benchmark_test_stop(void)
{
    // Check if this was called on slave node.
    if (m_state == TEST_IN_PROGRESS_WAITING_FOR_STOP_CMD)
    {
        return (uint32_t)zigbee_benchmark_test_stop_slave();
    }

    if (mp_test_configuration == NULL)
    {
        return (uint32_t)RET_ERROR;
    }

    if (m_state == TEST_IDLE)
    {
        LOG_INF("There is no ongoing test.");
        return (uint32_t)RET_ERROR;
    }

    LOG_INF("Reset benchmark state.");
    m_state = TEST_IDLE;
    benchmark_test_duration_calculate();
    mac_counters_calculate();
    cpu_utilization_stop();

    if (m_test_status.test_in_progress)
    {
        zb_uint16_t peer_addr = m_peer_information.peer_table[m_peer_information.selected_peer].p_address->nwk_addr;

        LOG_INF("Stop remote peer.");
        m_test_status.test_in_progress = false;

        return (uint32_t)zb_buf_get_out_delayed_ext(zigbee_benchmark_peer_stop_request_send, peer_addr, 0);
    }

    return (uint32_t) RET_OK;
}

uint32_t benchmark_peer_discover(void)
{
    if (m_state != TEST_IDLE)
    {
        LOG_INF("Stop current test in order to start peer discovery.");
        return (uint32_t)RET_ERROR;
    }

    if (mp_test_configuration == NULL)
    {
        LOG_INF("Provide test configuration before starting a peer discovery.");
        return (uint32_t)RET_ERROR;
    }

    memset(&m_peer_information, 0, sizeof(m_peer_information));

    zb_ret_t error = zigbee_benchmark_peer_discovery_request_send();
    if (error != RET_OK)
    {
        return (uint32_t)error;
    }

    k_timer_start(&discovery_timer, K_MSEC(BENCHMARK_DISCOVERY_TIMEOUT), K_NO_WAIT);

    return (uint32_t)RET_OK;
}

const benchmark_peer_db_t * benchmark_peer_table_get(void)
{
    return &m_peer_information;
}

benchmark_status_t * benchmark_status_get(void)
{
    return &m_test_status;
}

void benchmark_process(void)
{
    switch (m_state)
    {
        // fall through
        case TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ACK:
        case TEST_IN_PROGRESS_FRAME_SENT_WAITING_FOR_ECHO:
        case TEST_IN_PROGRESS_FRAME_SENDING:
        case TEST_IN_PROGRESS_WAITING_FOR_STOP_CMD:
        case TEST_IDLE:
            break;

        case TEST_ERROR:
            LOG_ERR("TEST_ERROR state");
            if (mp_test_configuration)
            {
                LOG_ERR("Error occurred during the test transmission. Sent %d packets.",
                        (mp_test_configuration->count - m_test_status.packets_left_count));
            }
            m_test_status.test_in_progress = false;
            m_state = TEST_IDLE;

            if (mp_callback)
            {
                m_benchmark_evt.evt           = BENCHMARK_TEST_STOPPED;
                m_benchmark_evt.context.error = (uint32_t)RET_ERROR;
                mp_callback(&m_benchmark_evt);
            }
            break;

        case TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER:
            // Retry sending the next buffer.
            LOG_INF("TEST_IN_PROGRESS_WAITING_FOR_TX_BUFFER state");
            zigbee_benchmark_send_ping_req();
            break;

        case TEST_IN_PROGRESS_FRAME_SENT:
            break;

        case TEST_FINISHED:
            LOG_INF("Benchmark test finished. Proceed with the tear down process.");
            benchmark_test_stop();
            break;

        default:
            LOG_INF("Invalid test state. Fall back to the idle state.");
            zigbee_benchmark_test_abort();
            break;
    }
}

uint64_t benchmark_local_device_id_get(void)
{
    eui64_map_addr_t dev_addr;

    zb_osif_get_ieee_eui64(dev_addr.arr_addr);

    return dev_addr.uint_addr;
}

uint32_t benchmark_peer_results_request_send(void)
{
    zb_uint16_t peer_addr = m_peer_information.peer_table[m_peer_information.selected_peer].p_address->nwk_addr;

    LOG_INF("Send a request for benchmark results to the remote peer.");
    return zb_buf_get_out_delayed_ext(zigbee_benchmark_peer_results_request_send, peer_addr, 0);
}

benchmark_result_t * zigbee_benchmark_local_result_get(void)
{
    return &m_local_result;
}
