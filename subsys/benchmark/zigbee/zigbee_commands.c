/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include <shell/shell.h>
#include <logging/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <benchmark_cli_util.h>
#include <zigbee/zigbee_app_utils.h>
#include "protocol_api.h"
#include "benchmark_zigbee_common.h"

#define LOG_BUF_SIZE (512)

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

void protocol_cmd_peer_db_get(const struct shell *shell, const benchmark_peer_db_t *p_peers)
{
    char peers_info[LOG_BUF_SIZE] = "# ||\tDevice ID\t|| 16-bit network address";
    char peers_info_len = strlen(peers_info);

    for (uint16_t i = 0; i < p_peers->peer_count; i++)
    {
        peers_info_len += sprintf(peers_info + peers_info_len,
                                  "\r\n[%d]:  %08x%08x  %04x",
                                  i,
                                  DEVICE_ID_HI(p_peers->peer_table[i].device_id),
                                  DEVICE_ID_LO(p_peers->peer_table[i].device_id),
                                  p_peers->peer_table[i].p_address->nwk_addr);
    }

    if (shell != NULL) {
        shell_info(shell, "\r\n%s", peers_info);
    } else {
        LOG_INF("\r\n%s", log_strdup(peers_info));
    }
}

void protocol_cmd_config_get(const struct shell *shell)
{
    zb_nwk_device_type_t role;
    zb_ieee_addr_t       addr;
    zb_uint16_t          short_addr;
    uint64_t             device_id = benchmark_local_device_id_get();

    zb_get_long_address(addr);
    short_addr = zb_address_short_by_ieee(addr);

    shell_info(shell, "\t=== Local node information ===");
    shell_info(shell, "Device ID:   %08x%08x",
               DEVICE_ID_HI(device_id),
               DEVICE_ID_LO(device_id));

    if (short_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
        shell_info(shell, "Network address: %04x", short_addr);
    }
    else
    {
        shell_info(shell, "Network address: none");
    }

    shell_info(shell, "Zigbee Role: ");
    role = zb_get_network_role();
    if (role == ZB_NWK_DEVICE_TYPE_NONE)
    {
        shell_info(shell, "unknown");
    }
    else if (role == ZB_NWK_DEVICE_TYPE_COORDINATOR)
    {
        shell_info(shell, "zc");
    }
    else if (role == ZB_NWK_DEVICE_TYPE_ROUTER)
    {
        shell_info(shell, "zr");
    }
    else if (role == ZB_NWK_DEVICE_TYPE_ED)
    {
        shell_info(shell, "zed");
    }
}

void protocol_cmd_peer_get(const struct shell *shell, const benchmark_peer_entry_t *p_peer)
{
    uint32_t device_id_lo = 0;
    uint32_t device_id_hi = 0;
    uint16_t short_addr   = 0xFFFF;

    if (p_peer)
    {
        device_id_lo = DEVICE_ID_LO(p_peer->device_id);
        device_id_hi = DEVICE_ID_HI(p_peer->device_id);
        short_addr   = p_peer->p_address->nwk_addr;
    }

    shell_info(shell, "\t=== Peer information ===");
    shell_info(shell, "Device ID:   %08x%08x", device_id_hi, device_id_lo);
    shell_info(shell, "Network address: %04x", short_addr);
}

static bool isdigit(char possible_digit)
{
    return (possible_digit <= '9') && (possible_digit >= '0');
}

static bool parse_tx_power(char *tx_power_str, int8_t *tx_power_parsed)
{
    uint32_t arg_len;
    bool is_negative;

    if (!tx_power_str) {
        goto parse_failed;
    }

    arg_len = strlen(tx_power_str);
    if (arg_len < 1 || arg_len > 4) {
        goto parse_failed;
    }

    is_negative = tx_power_str[0] == '-';
    if (!(is_negative || isdigit(tx_power_str[0]))) {
        goto parse_failed;
    }

    for (int i = (int)is_negative; i < arg_len; i++) {
        if (!isdigit(tx_power_str[i])) {
            goto parse_failed;
        }
    }

    *tx_power_parsed = atoi(tx_power_str);
    return true;

parse_failed:
    return false;
}

static void tx_power_cmd_local(const struct shell *shell, size_t argc, char **argv)
{
    zb_int8_t tx_power;

    if (argc == 1)
    {
        tx_power = zb_mac_get_tx_power();
        shell_info(shell, "Zigbee TX power is set to: %d dBm", tx_power);
    }
    else if (argc == 2 && parse_tx_power(argv[1], &tx_power))
    {
        zb_mac_set_tx_power(tx_power);
        shell_info(shell, "Setting the tx power to %d dBm", tx_power);
    }
    else
    {
        shell_error(shell, "Invalid value (%s) The tx power can be set to value between: <-127, 127>", argv[1]);
    }
}

static void tx_power_get_remote(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    zb_uint16_t peer_addr;

    if (parse_hex_u16(argv[1], &peer_addr)) {
        shell_info(shell, "Getting tx power from %.4x", peer_addr);
        zb_buf_get_out_delayed_ext(zigbee_benchmark_get_tx_power_remote, peer_addr, 0);
    } else {
        shell_error(shell, "Failed to parse remote peer's address");
    }
}

static void tx_power_set_remote(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    uint16_t peer_addr;
    int8_t tx_power;


    if (parse_hex_u16(argv[1], &peer_addr) && parse_tx_power(argv[2], &tx_power)) {
        shell_info(shell, "Setting tx power of %.4x to %d", peer_addr, tx_power);
        zigbee_benchmark_set_tx_power_remote(peer_addr, tx_power);
    } else {
        shell_error(shell, "Failed to parse remote peer's address");
    }
}

static void open_network_request(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    zb_buf_get_out_delayed(zigbee_benchmark_open_network_remote);

    shell_info(shell, "Requested opening the network");
}

static void device_reset_request(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    zb_uint16_t peer_addr;

    if (parse_hex_u16(argv[1], &peer_addr)) {
        shell_info(shell, "Reseting remote peer %.4x", peer_addr);
        zb_buf_get_out_delayed_ext(zigbee_benchmark_device_reset_remote, peer_addr, 0);
    } else {
        shell_error(shell, "Failed to parse remote peer's address");
    }
}

SHELL_STATIC_SUBCMD_SET_CREATE(bm_zigbee_extension,
    SHELL_CMD_ARG(tx_power_local, NULL, "Get/Set tx power of the local device", tx_power_cmd_local, 1, 1),
    SHELL_CMD_ARG(tx_power_get,   NULL, "Get radio power of the remote peer", tx_power_get_remote, 2, 0),
    SHELL_CMD_ARG(tx_power_set,   NULL, "Set radio power of the remote peer", tx_power_set_remote, 3, 0),
    SHELL_CMD_ARG(open_network,   NULL, "Request the network coordinator to open the network", open_network_request, 1, 0),
    SHELL_CMD_ARG(device_reset,   NULL, "Reset remote device", device_reset_request, 2, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(test_zigbee, &bm_zigbee_extension, "Extended benchmark Zigbee commands", NULL);
