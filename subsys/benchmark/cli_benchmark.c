/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include <assert.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <logging/log.h>

#include <benchmark_cli_util.h>
#include "protocol_api.h"
// #include "cpu_utilization.h"
#include "cli_suppress.h"

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

#define DECIMAL_PRECISION 100
#define BPS_TO_KBPS       1024

#define LOG_BUFFER_STRDUP_NUM 8
#define LOG_BUFFER_SIZE   (1024)

typedef struct log_buffer {
    uint16_t buf_usage;
    char buf[LOG_BUFFER_SIZE];
} log_buffer_t;

static void print_test_results(benchmark_event_context_t * p_context);

static log_buffer_t results_log_buf;
struct shell const *p_shell;
static benchmark_peer_db_t *mp_peer_db;
static benchmark_configuration_t m_test_configuration =
{
    .length       = 64,
    .ack_timeout  = 200,
    .count        = 1000,
    .mode         = BENCHMARK_MODE_ACK,
};

static void log_buffer_flush(log_buffer_t *log_buf)
{
    printk("\r\n%s", log_strdup(log_buf->buf));

    log_buf->buf_usage = 0;
    memset(log_buf->buf, 0, LOG_BUFFER_SIZE);
}

/**
 * @brief Check if log_buffer has sufficient space to put <bytes_needed> of bytes.
 *
 * @param log_buf points to buffer to check
 * @param bytes_needed number of bytes needed to put into buffer
 * @param flush if set to true, remaining bytes are flushed
 * @return true if has sufficient space or space is available after flush
 * @return false if not sufficient space is available and flush wasn't required
 */
static bool log_buffer_has_space(log_buffer_t *log_buf, uint16_t bytes_needed, bool flush)
{
    bool has_space = (log_buf->buf_usage + bytes_needed) < LOG_BUFFER_SIZE;

    if (!has_space && flush)
    {
        log_buffer_flush(log_buf);

        has_space = bytes_needed < LOG_BUFFER_SIZE;
    }

    return has_space;
}

static bool shell_help_requested(const struct shell *shell, int argc, char **argv)
{
    return strcmp(argv[0], "help") == 0;
}

static const char *configuration_mode_name_get(benchmark_mode_t * p_mode)
{
    switch (*(p_mode))
    {
        case BENCHMARK_MODE_UNIDIRECTIONAL:
        return "Unidirectional";

        case BENCHMARK_MODE_ECHO:
        return "Echo";

        case BENCHMARK_MODE_ACK:
        return "Ack";

        default:
        return "Unknown mode";
    }
}

static void discovered_peers_print(const struct shell *shell)
{
    if (mp_peer_db != NULL)
    {
        protocol_cmd_peer_db_get(shell, mp_peer_db);
    }
    else
    {
        print_error(shell, "The list of known peers is empty. Discover peers and try again.");
    }
}

static void benchmark_evt_handler(benchmark_evt_t * p_evt)
{
    uint16_t peer_count;

    switch (p_evt->evt)
    {
        case BENCHMARK_TEST_COMPLETED:
            LOG_INF("Test completed.");
            print_test_results(&(p_evt->context));
            break;

        case BENCHMARK_TEST_STARTED:
            assert(!protocol_is_error(p_evt->context.error));
            LOG_INF("Test started.");
            cli_suppress_enable();
            break;

        case BENCHMARK_TEST_STOPPED:
            cli_suppress_disable();

            if (!protocol_is_error(p_evt->context.error))
            {
                LOG_INF("Test successfully stopped.");
            }
            else
            {
                LOG_INF("Test stopped with errors. Error code: %u", p_evt->context.error);
                if (p_shell)
                {
                    shell_error(p_shell, "Error: Test stopped with errors. Error code: %u\r\n", p_evt->context.error);
                }
            }
            break;

        case BENCHMARK_DISCOVERY_COMPLETED:
            peer_count = p_evt->context.p_peer_information->peer_count;
            mp_peer_db = p_evt->context.p_peer_information;

            if (peer_count)
            {
                mp_peer_db->selected_peer  = peer_count - 1;
            }
            else
            {
                mp_peer_db = NULL;
            }

            LOG_INF("Discovery completed, found %d peers.", peer_count);

            discovered_peers_print(NULL);
            break;

        default:
            LOG_DBG("Unknown benchmark_evt.");
            break;
    };
}

const benchmark_peer_entry_t * benchmark_peer_selected_get(void)
{
    if (mp_peer_db == NULL)
    {
        return NULL;
    }

    return &(mp_peer_db->peer_table[mp_peer_db->selected_peer]);
}

/** Common commands, API used by all benchmark commands */
void print_done(const struct shell *shell)
{
    LOG_INF("Done");
}

void print_error(const struct shell *shell, char *what)
{
    LOG_ERR("Error: %s", what);
}

void cmd_default(const struct shell *shell, size_t argc, char ** argv)
{
    if ((argc == 1) || shell_help_requested(shell, argc, argv))
    {
        shell_help(shell);
    }
    else
    {
        shell_error(shell, "Error: %s: unknown parameter: %s\r\n", argv[0], argv[1]);
    }
}

/** Test configuration commands */
void cmd_config_get(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 1)
    {
        // If unknown subcommand was passed.
        cmd_default(shell, argc, argv);
        return;
    }

    shell_info(shell, "=== Test settings ==="
                      "\nMode:%10s"
                      "\nACK Timeout:%10d [ms]"
                      "\nPacket count:%10d"
                      "\nPayload length [B]:%10d",
                      configuration_mode_name_get(&m_test_configuration.mode),
                      m_test_configuration.ack_timeout,
                      m_test_configuration.count,
                      m_test_configuration.length);
}

static void cmd_info_get(const struct shell *shell, size_t argc, char ** argv)
{
    // Test settings
    cmd_config_get(shell, 0, NULL);

    // Local node information
    protocol_cmd_config_get(shell);

    // Peer information
    protocol_cmd_peer_get(shell, benchmark_peer_selected_get());
}

static void cmd_config_mode_get(const struct shell *shell, size_t argc, char **argv)
{
    if (argc > 1)
    {
        // If unknown subcommand was passed.
        cmd_default(shell, argc, argv);
        return;
    }

    shell_info(shell, "%s\r\n", configuration_mode_name_get(&m_test_configuration.mode));
}

static void cmd_config_mode_unidirectional_set(const struct shell *shell, size_t argc, char ** argv)
{
    m_test_configuration.mode = BENCHMARK_MODE_UNIDIRECTIONAL;
}

static void cmd_config_mode_echo_set(const struct shell *shell, size_t argc, char **argv)
{
    m_test_configuration.mode = BENCHMARK_MODE_ECHO;
}

static void cmd_config_mode_ack_set(const struct shell *shell, size_t argc, char **argv)
{
    m_test_configuration.mode = BENCHMARK_MODE_ACK;
}

static void cmd_config_ack_timeout(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1)
    {
        shell_info(shell,"%d", m_test_configuration.ack_timeout);
        return;
    }

    if (argc == 2)
    {
        m_test_configuration.ack_timeout = atoi(argv[1]);
    }
}

static void cmd_config_packet_count(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 2)
    {
        print_error(shell, "Too many arguments\r\n");
        return;
    }

    if (argc < 2)
    {
        shell_info(shell,"%d\r\n", m_test_configuration.count);
    }
    else if (argc == 2)
    {
        m_test_configuration.count = atoi(argv[1]);
    }
}

static void cmd_config_packet_length(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 2)
    {
        print_error(shell, "Too many arguments.");
        return;
    }

    if (argc < 2)
    {
        shell_info(shell,"%d", m_test_configuration.length);
    }
    else if (argc == 2)
    {
        m_test_configuration.length = atoi(argv[1]);
    }
}

/** Peer configuration commands */
static void cmd_discover_peers(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code;

    // Remember cli used to start the test so results can be printed on the same interface.
    p_shell = shell;

    err_code = benchmark_test_init(&m_test_configuration, benchmark_evt_handler);
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to configure discovery parameters.");
    }

    err_code = benchmark_peer_discover();
    if (protocol_is_error(err_code))
    {
        shell_error(shell, "Failed to sent discovery message.");
    }
}

static void cmd_display_peers(const struct shell *shell, size_t argc, char **argv)
{
    discovered_peers_print(shell);
}

static void cmd_peer_select(const struct shell *shell, size_t argc, char **argv)
{
    if (mp_peer_db == NULL)
    {
        print_error(shell, "The list of known peers is empty. Discover peers and try again.");
        return;
    }

    if (argc > 2)
    {
        print_error(shell, "Too many arguments.");
        return;
    }

    if (argc == 1)
    {
        shell_info(shell, "%d\r\n", mp_peer_db->selected_peer);
        return;
    }

    if (mp_peer_db->peer_count > atoi(argv[1]))
    {
        mp_peer_db->selected_peer = atoi(argv[1]);
    }
    else
    {
        print_error(shell, "Peer index out of range.\n");
    }
}

static void parse_decimal(char *buf_out, const char *p_description, const char *p_unit, uint32_t value)
{
    if (value != BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED)
    {
        uint32_t value_int       = value / DECIMAL_PRECISION;
        uint32_t value_remainder = value % DECIMAL_PRECISION;

        sprintf(buf_out, "%s: %lu.%02lu%s", p_description, value_int, value_remainder, p_unit);
    }
    else
    {
        LOG_INF("%s: Not supported\n", p_description);
    }
}

static void dump_config(benchmark_configuration_t * p_config)
{
    const char *const modes[] = {"Unidirectional", "Echo", "ACK"};

    LOG_INF("\r\n        Length: %u"
             "\r\n        ACK timeout: %u ms"
             "\r\n        Count: %u"
             "\r\n        Mode: %s",
             p_config->length, p_config->ack_timeout,
             p_config->count, modes[p_config->mode]);
}

static void dump_status(benchmark_status_t * p_status)
{
    LOG_INF("\r\n        Test in progress: %s"
             "\r\n        Reset counters: %s"
             "\r\n        ACKs lost: %u"
             "\r\n        Waiting for ACKs: %u"
             "\r\n        Packets left count: %u"
             "\r\n        Frame number: %u",
             p_status->test_in_progress ? "True" : "False",
             p_status->reset_counters ? "True" : "False",
             p_status->acks_lost,
             p_status->waiting_for_ack,
             p_status->packets_left_count,
             p_status->frame_number);

    if (m_test_configuration.mode == BENCHMARK_MODE_ECHO)
    {
        uint32_t avg = 0;
        static char latency_min[25] = "";
        static char latency_max[25] = "";
        static char latency_avg[25] = "";

        if (p_status->latency.cnt > 0)
        {
            avg = (uint32_t)(p_status->latency.sum * DECIMAL_PRECISION / (p_status->latency.cnt));
        }

        parse_decimal(latency_min, "", "", p_status->latency.min * DECIMAL_PRECISION);
        parse_decimal(latency_max, "", "", p_status->latency.max * DECIMAL_PRECISION);
        parse_decimal(latency_avg, "", "", avg);

        LOG_INF("\r\n        Latency:"
                "\r\n            Avg: %s ms"
                "\r\n            Max: %s ms"
                "\r\n            Min: %s ms",
                log_strdup(latency_avg), log_strdup(latency_max), log_strdup(latency_min));
    }
}

static void dump_result(benchmark_result_t * p_result)
{
    LOG_INF("\r\nDuration: %u ms"
            "\r\nApp counters:"
            "\r\n    Bytes received: %uB"
            "\r\n    Packets received: %u"
            "\r\n    RX error: %u"
            "\r\n    RX total: %u"
            "\r\nMac counters:"
            "\r\n    TX error: %u"
            "\r\n    TX total: %u",
            p_result->duration,
            p_result->rx_counters.bytes_received,
            p_result->rx_counters.packets_received,
            p_result->rx_counters.rx_error,
            p_result->rx_counters.rx_total,
            p_result->mac_tx_counters.error,
            p_result->mac_tx_counters.total);
}

#if CONFIG_THREAD_RUNTIME_STATS

static void foreach_callback(const struct k_thread *thread, void *user_data)
{
	k_thread_runtime_stats_t stats;
    uint32_t all_ticks = k_cycle_get_32();
	int ret;

	k_thread_runtime_stats_get((k_tid_t)thread, &stats);
	((k_thread_runtime_stats_t *)user_data)->execution_cycles +=
		stats.execution_cycles;

    printk("Thread: \"%s\" used %llu (%f.2\%)\n", k_thread_name_get((k_tid_t)thread),
                                                stats.execution_cycles,
                                                stats.execution_cycles*100.0/all_ticks);
}

static void test_thread_runtime_stats_get(void)
{
	k_thread_runtime_stats_t stats, stats_all;
	int ret;

	stats.execution_cycles = 0;
	k_thread_foreach(foreach_callback, &stats);

	/* Check NULL parameters */
	ret = k_thread_runtime_stats_all_get(NULL);

	k_thread_runtime_stats_all_get(&stats_all);
}

#endif

/** Test execution commands */
static void print_test_results(benchmark_event_context_t * p_context)
{
    benchmark_evt_results_t *p_results = &p_context->results;

    if (p_results->p_remote_result == NULL)
    {
        return;
    }

    LOG_INF("Test Finished!!");

    if ((p_results->p_local_status != NULL) && (p_results->p_local_result != NULL) && (p_results->p_local_result->duration != 0))
    {
        uint32_t test_duration                            = p_results->p_local_result->duration;
        uint32_t packets_sent                             = m_test_configuration.count - p_results->p_local_status->packets_left_count;
        uint32_t packets_acked                            = packets_sent - p_results->p_local_status->acks_lost;
        uint32_t txed_bytes                               = m_test_configuration.length * packets_sent;
        uint32_t acked_bytes                              = m_test_configuration.length * packets_acked;
        uint32_t throughput                               = (uint32_t)((txed_bytes*8) / (test_duration));
        uint32_t throughput_rtx                           = (uint32_t)((acked_bytes*8) / (test_duration));

        (void)log_buffer_has_space(&results_log_buf, 100, true);
        results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                           "\r\nPackets sent: %u\r\nPackets acked: %u\r\nBytes sent: %u\r\nBytes acked: %u"
                           "\r\nTest duration: %ums",
                           packets_sent, packets_acked, txed_bytes, acked_bytes, test_duration);

        if (m_test_configuration.mode == BENCHMARK_MODE_ECHO)
        {
            char avg_str[40] = "";
            uint32_t avg = 0;

            if (p_results->p_local_status->latency.cnt > 0)
            {
                avg = (uint32_t)(p_results->p_local_status->latency.sum * DECIMAL_PRECISION / p_results->p_local_status->latency.cnt);
            }

            parse_decimal(avg_str, "    Avg", "", avg);

            (void)log_buffer_has_space(&results_log_buf, 60, true);
            results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                               "\r\nLatency:"
                               "\r\n%s"
                               "\r\n    Min: %u"
                               "\r\n    Max: %u",
                               avg_str,
                               p_results->p_local_status->latency.min,
                               p_results->p_local_status->latency.max);
        }

        if (m_test_configuration.mode == BENCHMARK_MODE_UNIDIRECTIONAL)
        {
            (void)log_buffer_has_space(&results_log_buf, 40, true);
            results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                               "\r\nUnidirectional:"
                               "\r\n    Throughput: %lukbps", throughput);
        }
        else
        {
            uint32_t per = UINT32_MAX;
            char per_str[2][40] = {"", ""};

            if (packets_sent != 0)
            {
                per = (uint32_t)((DECIMAL_PRECISION * 100ULL * (packets_sent - packets_acked)) / packets_sent);
            }

            parse_decimal(per_str[0], "    PER", "%", per);
            parse_decimal(per_str[1], "    PER", "%", 0);

            (void)log_buffer_has_space(&results_log_buf, 150, true);
            results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                                                 "\r\nWithout retransmissions:"
                                                 "\r\n%s" "\r\n    Throughput: %lukbps"
                                                 "\r\nWith retransmissions:"
                                                 "\r\n%s" "\r\n    Throughput: %lukbps",
                                                 per_str[0], throughput, per_str[1], throughput_rtx);
        }

        if (m_test_configuration.mode == BENCHMARK_MODE_UNIDIRECTIONAL)
        {
            uint32_t mac_tx_attempts = p_results->p_local_result->mac_tx_counters.total;
            uint32_t mac_tx_errors   = p_results->p_local_result->mac_tx_counters.error;
            uint32_t mac_per         = UINT32_MAX;
            char mac_per_str[40]     = "";

            if (mac_tx_attempts != 0)
            {
                mac_per = (uint32_t)((DECIMAL_PRECISION * 100ULL * mac_tx_errors) / mac_tx_attempts);
            }

            parse_decimal(mac_per_str, "MAC PER", "%", mac_per);
            (void)log_buffer_has_space(&results_log_buf, 30, true);
            results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage, "\r\n%s", mac_per_str);
        }
        else
        {
            if (p_results->p_remote_result != NULL)
            {
                uint32_t mac_tx_attempts = p_results->p_local_result->mac_tx_counters.total + p_results->p_remote_result->mac_tx_counters.total;
                uint32_t mac_tx_errors   = p_results->p_local_result->mac_tx_counters.error + p_results->p_remote_result->mac_tx_counters.error;
                uint32_t mac_per         = UINT32_MAX;
                char mac_per_str[40]     = "";

                if (mac_tx_attempts != 0)
                {
                    mac_per = (uint32_t)((DECIMAL_PRECISION * 100ULL * mac_tx_errors) / mac_tx_attempts);
                }

                parse_decimal(mac_per_str, "MAC PER", "%", mac_per);

                (void)log_buffer_has_space(&results_log_buf, 40, true);
                results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage, "\r\n%s", mac_per_str);
            }
            else
            {
                (void)log_buffer_has_space(&results_log_buf, 60, true);
                results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                                   "\r\nMAC Counters:"
                                   "\r\n    MAC TX Total: %u"
                                   "\r\n    MAC TX Err: %u",
                                   p_results->p_local_result->mac_tx_counters.total,
                                   p_results->p_local_result->mac_tx_counters.error);
                // LOG_INF("MAC Counters:");
                // print_int(p_shell, "    MAC TX Total", "", p_results->p_local_result->mac_tx_counters.total);
                // print_int(p_shell, "    MAC TX Err", "",   p_results->p_local_result->mac_tx_counters.error);
            }
        }

        (void)log_buffer_has_space(&results_log_buf, 30, true);
        results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                                             "\r\nRaw data:"
                                             "\r\n    Config:");
        dump_config(&m_test_configuration);

        (void)log_buffer_has_space(&results_log_buf, 20, true);
        results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage,
                           "\r\n    Status:");
        dump_status(p_results->p_local_status);

        (void)log_buffer_has_space(&results_log_buf, 10, true);
        results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage, "\r\nLocal:");
        dump_result(p_results->p_local_result);

        if (p_results->p_remote_result != NULL)
        {
            (void)log_buffer_has_space(&results_log_buf, 10, true);
            results_log_buf.buf_usage += sprintf(results_log_buf.buf + results_log_buf.buf_usage, "\r\nRemote:");
            dump_result(p_results->p_remote_result);
        }
    }

#if CONFIG_THREAD_RUNTIME_STATS
    test_thread_runtime_stats_get();
#endif

    log_buffer_flush(&results_log_buf);
}

static void cmd_test_start(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code;

    if (mp_peer_db == NULL)
    {
        shell_error(shell, "No peer selected; run:\n     test peer discover \nto find peers\n");
        return;
    }

    // Remember cli used to start the test so results can be printed on the same interface.
    p_shell = shell;

    err_code = benchmark_test_init(&m_test_configuration, benchmark_evt_handler);
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to configure test parameters.");
        return;
    }

    err_code = benchmark_test_start();
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to start test.");
    }
}

static void cmd_test_stop(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code = benchmark_test_stop();
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to stop test.");
    }
}

static void cmd_peer_test_results(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code = benchmark_peer_results_request_send();

    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to send test results request.");
        return;
    }
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_configure_mode,
    SHELL_CMD_ARG(ack, NULL, "Peer replies with a short acknowledgment",
                  cmd_config_mode_ack_set, 1, 0),
    SHELL_CMD_ARG(echo, NULL, "Peer replies with the same data",
                  cmd_config_mode_echo_set, 1, 0),
    SHELL_CMD_ARG(unidirectional, NULL, "Transmission in the single direction",
                  cmd_config_mode_unidirectional_set, 1, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(test_configure_peer,
    SHELL_CMD_ARG(discover, NULL, "Discover available peers", cmd_discover_peers, 1, 0),
    SHELL_CMD_ARG(list,     NULL, "Display found peers",      cmd_display_peers, 1, 0),
    SHELL_CMD_ARG(results,  NULL, "Display results",          cmd_peer_test_results, 1, 0),
    SHELL_CMD_ARG(select,   NULL, "Select peer from a list",  cmd_peer_select, 1, 1),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(test_configure_cmds,
    SHELL_CMD_ARG(ack-timeout, NULL,
        "Set time after we stop waiting for the acknowledgment from the peer in milliseconds",
                  cmd_config_ack_timeout, 1, 1),
    SHELL_CMD_ARG(count, NULL, "Set number of packets to be sent",
                  cmd_config_packet_count, 1, 1),
    SHELL_CMD_ARG(length, NULL, "Set UDP payload length in bytes",
                  cmd_config_packet_length, 1, 1),
    SHELL_CMD_ARG(mode, &test_configure_mode, "Set test type",
                  cmd_config_mode_get, 2, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(benchmark_cmds,
    SHELL_CMD_ARG(configure, &test_configure_cmds, "Configure the test",
                  cmd_default, 1, 1),
    SHELL_CMD_ARG(info, NULL, "Print current configuration", cmd_info_get, 1, 0),
    SHELL_CMD_ARG(peer, &test_configure_peer, "Configure the peer receiving data",
                  cmd_default, 1, 0),
    SHELL_CMD_ARG(start, NULL, "Start the test", cmd_test_start, 1, 0),
    SHELL_CMD_ARG(stop, NULL, "Stop the test", cmd_test_stop, 1, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(test, &benchmark_cmds, "Benchmark commands", cmd_default);
