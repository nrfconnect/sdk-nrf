/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include <logging/log.h>
#include "cli_suppress.h"

#include "cli_api.h"

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

static bool m_suppress_cli = false;

void cli_suppress_enable(void)
{
    LOG_DBG("Suppressing CLI");

    m_suppress_cli = true;
}

void cli_suppress_disable(void)
{
    m_suppress_cli = false;
    LOG_DBG("Enabling CLI");
}

bool cli_suppress_is_enabled(void)
{
    return m_suppress_cli;
}
