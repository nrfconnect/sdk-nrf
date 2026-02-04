.. _lib_at_shell:

AT shell
########

.. contents::
   :local:
   :depth: 2

You can use this library to add an AT shell command to a shell.
This enables you to send AT commands to the modem and receive responses.

Configuration
*************

The library expects that you have enabled a shell using :kconfig:option:`CONFIG_SHELL`.
To enable and configure the AT shell, use the following Kconfig options:

* :kconfig:option:`CONFIG_AT_SHELL` - Enables the AT shell library
* :kconfig:option:`CONFIG_AT_SHELL_CMD_RESPONSE_MAX_LEN` - Maximum AT command response length
* :kconfig:option:`CONFIG_AT_SHELL_CMD_MODE` - Enables AT command mode (shell bypass)
* :kconfig:option:`CONFIG_AT_SHELL_CMD_MODE_TERMINATION` - Default line termination in AT command mode (default is CR)
* :kconfig:option:`CONFIG_AT_SHELL_CMD_MODE_ECHO_DEFAULT` - Default local echo in AT command mode

To send AT read commands (ending with ``?``), disable the shell wildcards using the following Kconfig option:

* :kconfig:option:`CONFIG_SHELL_WILDCARD`

Usage
*****

When the library is enabled, the shell will have a command ``at``.
To send AT commands, use the following syntax:

  .. code-block:: console

     at AT+CEREG?

Known subcommands are registered as shell subcommands, so ``at <Tab>`` completes them.

You can also enable AT command notifications and disable them.

* Enable AT command notifications:

  .. code-block:: console

     at events_enable

* Disable AT command notifications:

  .. code-block:: console

     at events_disable

AT command mode is an interactive shell bypass where input is forwarded as AT commands.
Output is printed using ``printk()`` because the shell is bypassed in raw mode.

* Start AT command mode:

  .. code-block:: console

     at at_cmd_mode start

Dependencies
************

This library uses the following |NCS| library:

* :ref:`at_monitor_readme`

This library uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem_at`

API documentation
*****************

| Header file: :file:`include/modem/at_shell.h`
| Source files: :file:`lib/at_shell/`

.. doxygengroup:: at_shell
