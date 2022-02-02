/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <shell/shell.h>
#include <modem/lte_lc.h>

int cmd_shutdown_modem(const struct shell *shell, size_t argc, char **argv)
{
    int err = 0;
    err = lte_lc_power_off();
    if (err == 0) {
      printk("The modem powered off\n");
    }
    else {
        printk("\n***** WARNING! Modem did not shutdown correctly. error: %d *****\n", err);
    }

    return 0;
}

SHELL_CMD_REGISTER(shutdown_modem, NULL, "Turn off modem to do a clean detaach from carreir network", cmd_shutdown_modem);
