/*$$$LICENCE_NORDIC_STANDARD<2022>$$$*/
#include <logging/log.h>
#include <shell/shell.h>

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

static bool m_suppress_cli = false;
extern struct shell *p_shell;

void cli_suppress_init(struct shell *_shell)
{
    p_shell = _shell;
}

void cli_suppress_enable(void)
{
    shell_info(p_shell, "Suppressing CLI");

    m_suppress_cli = true;

    shell_stop(p_shell);
}

void cli_suppress_disable(void)
{
    m_suppress_cli = false;

    shell_start(p_shell);

    shell_info(p_shell, "Enabling CLI");
}

bool cli_suppress_is_enabled(void)
{
    return m_suppress_cli;
}
