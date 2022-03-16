/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "benchmark_api.h"
#include "nrf_cli.h"
#include "cli_api.h"
#include "commands.h"
#include "protocol_api.h"


static void cmd_remote_attach(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    const benchmark_peer_db_t * p_peer_db = benchmark_peer_table_get();

    if (p_peer_db->peer_count == 0)
    {
        print_error(p_cli, "The list of known peers is empty. Discover peers and try again");
        return;
    }

    for (size_t i = 0; i < CLI_REMOTE_INSTANCES && i < p_peer_db->peer_count; i++)
    {
        cli_remote_peer_set(cli_remote_get(i),
                            p_peer_db->peer_table[i].p_address);
    }
}

static void cmd_remote(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    nrf_cli_t const * p_peer_cli;

    if (argc < 3)
    {
       nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Required arguments not provided\r\n");
       return;
    }
    else
    {
        p_peer_cli = cli_remote_get(atoi(argv[1]));

        if (p_peer_cli == NULL)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Remote number out of range\r\n");
            return;
        }
    }

    protocol_cmd_remote_send(p_cli, p_peer_cli, argc - 2, &(argv[2]));
}


NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_remote_cmds)
{
    NRF_CLI_CMD(attach, NULL, "Attach remote CLI connections to all available peers", cmd_remote_attach),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(remote, &m_remote_cmds, "Send command to remote peer", cmd_remote);
