/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "cli_api.h"
#include <zboss_api.h>
#include <zigbee_cli.h>

void cli_init(void)
{
    /* Initialize the Zigbee CLI subsystem */
    zb_cli_init(ZIGBEE_CLI_ENDPOINT);
}

void cli_remote_init(void)
{
    /* Remote command execution is not implemented in Zigbee benchmark. */
    return;
}

void cli_start(void)
{
    /* Start Zigbee CLI subsystem. */
    zb_cli_start();

    /* Set the endpoint receive hook */
    ZB_AF_SET_ENDPOINT_HANDLER(ZIGBEE_CLI_ENDPOINT, cli_agent_ep_handler);
}

void cli_remote_start(void)
{
    /* Remote command execution is not implemented in Zigbee benchmark. */
    return;
}

void cli_process(void)
{
    UNUSED_RETURN_VALUE(zb_cli_process());
}

nrf_cli_t const * cli_remote_get(size_t idx)
{
    /* Remote command execution is not implemented in Zigbee benchmark. */
    return NULL;
}

void cli_remote_peer_set(nrf_cli_t const * p_peer_cli, benchmark_address_context_t * p_peer_address)
{
    /* Remote command execution is not implemented in Zigbee benchmark. */
    return;
}
