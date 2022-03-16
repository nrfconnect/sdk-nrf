/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "cpu_utilization.h"
#include "cli_api.h"
#include "nrf_cli.h"
#include "commands.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


static void cmd_cpu_utilization_init(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    uint32_t err = cpu_utilization_init();

    if (err != NRF_SUCCESS)
    {
        print_error(p_cli, "Failed to initialize CPU utilization module");
    }
    else
    {
        print_done(p_cli);
    }
}

static void cmd_cpu_utilization_deinit(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    uint32_t err = cpu_utilization_deinit();

    if (err != NRF_SUCCESS)
    {
        print_error(p_cli, "Failed to deinitialize CPU utilization module");
    }
    else
    {
        print_done(p_cli);
    }
}

static void cmd_cpu_utilization_start(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    uint32_t err = cpu_utilization_start();

    if (err != NRF_SUCCESS)
    {
        print_error(p_cli, "Failed to start CPU utilization module");
    }
    else
    {
        print_done(p_cli);
    }
}

static void cmd_cpu_utilization_stop(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    uint32_t err = cpu_utilization_stop();

    if (err != NRF_SUCCESS)
    {
        print_error(p_cli, "Failed to initialize CPU utilization module");
    }
    else
    {
        print_done(p_cli);
    }
}

static void cmd_cpu_utilization_get(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    uint32_t cpu_util_value = cpu_utilization_get();

    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "CPU Utilization: %d.%02d%%\r\n", (cpu_util_value / 100), (cpu_util_value % 100));
    print_done(p_cli);
}

static void cmd_cpu_utilization_clear(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    cpu_utilization_clear();
    print_done(p_cli);
}

NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_cpu_utilization_cmds)
{
    NRF_CLI_CMD(clear,  NULL, "Clear CPU utilization measurement",        cmd_cpu_utilization_clear),
    NRF_CLI_CMD(deinit, NULL, "Deinitialize CPU utilization measurement", cmd_cpu_utilization_deinit),
    NRF_CLI_CMD(get,    NULL, "Get current CPU utilization measurement",  cmd_cpu_utilization_get),
    NRF_CLI_CMD(init,   NULL, "Initialize CPU utilization measurement",   cmd_cpu_utilization_init),
    NRF_CLI_CMD(start,  NULL, "Start CPU utilization measurement",        cmd_cpu_utilization_start),
    NRF_CLI_CMD(stop,   NULL, "Stop CPU utilization measurement",         cmd_cpu_utilization_stop),
    NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(cpu_utilization, &m_cpu_utilization_cmds, "CPU Utilization commands", cmd_default);
