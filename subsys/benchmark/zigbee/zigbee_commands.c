/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include <shell/shell.h>
#include <logging/log.h>
#include <string.h>

#include <benchmark_cli_util.h>
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

void protocol_cmd_remote_send(const struct shell *shell, const nrf_cli_t * p_peer_cli, size_t argc, char ** argv)
{
    /* Remote command execution is not implemented in Zigbee benchmark. */
    return;
}

static bool isdigit(char possible_digit)
{
    return (possible_digit <= '9') && (possible_digit >= '0');
}

static void tx_power_cmd(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1)
    {
        zb_int8_t tx_power = zb_mac_get_tx_power();

        shell_info(shell, "Zigbee TX power is set to: %d dBm", tx_power);
    }

    if (argc == 2)
    {
        int8_t tx_power;
        char *new_tx_power = argv[1];
        uint32_t arg_len = strlen(argv[1]);
        bool is_negative = new_tx_power[0] == '-';

        if (arg_len > 4) {
            goto parse_failed;
        }

        if (!(is_negative || isdigit(new_tx_power[0]))) {
            goto parse_failed;
        }

        for (int i = (int)is_negative; i < arg_len; i++) {
            if (!isdigit(new_tx_power[i])) {
                goto parse_failed;
            }
        }

        tx_power = atoi(new_tx_power);
        zb_mac_set_tx_power(tx_power);

        shell_info(shell, "Setting the tx power to %d dBm", tx_power);
    }

    return;
parse_failed:
    shell_error(shell, "Invalid value (%s) The tx power can be set to value between: <-127, 127>", argv[1]);
}

SHELL_STATIC_SUBCMD_SET_CREATE(bm_zigbee_extension,
    SHELL_CMD_ARG(tx_power, NULL, "Get/Set radio power", tx_power_cmd, 1, 1),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(test_zigbee, &bm_zigbee_extension, "Extended benchmark Zigbee commands", NULL);
