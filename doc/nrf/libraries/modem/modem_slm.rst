.. _lib_modem_slm:

Modem SLM
#########

.. contents::
   :local:
   :depth: 2

The Modem SLM library exposes the AT command interface of the :ref:`Serial LTE Modem <slm_description>` application for external devices over a serial interface.
This library is for applications running on external MCU that is built with |NCS| and is connected to nRF9160 SiP through UART.

Overview
********

The Modem SLM library allows you to perform the following functions:

* Manage the serial interface so that the application only decides which UART device to use and configures its DTS.
* Manage the GPIO pins, with support for bidirectional indication and wakeup.
* Send modem or SLM proprietary AT commands, receive responses and notifications, similar to the :ref:`lib_at_host` library.
  Received AT responses or notifications can be parsed by the :ref:`at_cmd_parser_readme` library.
* Send raw data in SLM data mode.
  Refer to :ref:`slm_data_mode`.
* Monitor AT notifications with registered callbacks, similar to the :ref:`at_monitor_readme` library.
* Send AT commands via UART or RTT shell, similar to the :ref:`lib_at_shell` library.

Configuration
*************

The library is enabled and configured entirely using the Kconfig system.

Configure the following Kconfig options to enable this library:

* :kconfig:option:`CONFIG_MODEM_SLM` - Enables the Modem SLM library.
* :kconfig:option:`CONFIG_MODEM_SLM_DMA_MAXLEN` - Configures UART RX EasyDMA buffer size, which is configured to 1024 bytes by default.
* :kconfig:option:`CONFIG_MODEM_SLM_WAKEUP_PIN` - Configures the mandatory wake-up GPIO, which is not configured by default.
* :kconfig:option:`CONFIG_MODEM_SLM_WAKEUP_TIME` - Sets the toggle time value in milliseconds for wake-up GPIO, by default 100 ms.

Optionally configure the following Kconfig options based on need:

* :kconfig:option:`CONFIG_MODEM_SLM_SHELL` - Enables the shell function in the Modem SLM library, which is not enabled by default.
* :kconfig:option:`CONFIG_MODEM_SLM_INDICATE_PIN` - Configures the optional indicator GPIO, which is not configured by default.

The application must use Zephyr ``chosen`` nodes in devicetree to select UART device.
Additionally, GPIO can also be selected.
For example:

.. code-block:: devicetree

   / {
      chosen {
         ncs,slm-uart = &uart1;
         ncs,slm-gpio = &gpio0;
      };
   };

Use one of the following options to select the termination character:

* :kconfig:option:`CONFIG_MODEM_SLM_CR_TERMINATION` - Enables ``<CR>`` as the termination character.
* :kconfig:option:`CONFIG_MODEM_SLM_LF_TERMINATION` - Enables ``<LF>`` as the termination character.
* :kconfig:option:`CONFIG_MODEM_SLM_CR_LF_TERMINATION` - Enables ``<CR+LF>`` as the termination character, which is selected by default.

You must configure the same termination character as that configured in SLM on nRF9160 SiP.
The library sends the termination character automatically after an AT command.

Shell usage
***********

To send AT commands in shell, use the following syntax:

  .. code-block:: console

     uart:~$ slm AT%XPTW=4,\"0001\"

     OK

     uart:~$ slm at%ptw?

     %XPTW: 4,"0001"
     %XPTW: 5,"0011"

     OK

SLM accepts AT command characters in upper, lower, or mixed case.

SLM Monitor usage
*****************

The SLM Monitor has similar functions to the :ref:`at_monitor_readme` library, except "Direct dispatching".

  .. code-block:: console

     SLM_MONITOR(network, "\r\n+CEREG:", cereg_mon);

     SLM_MONITOR(download, "\r\n#XDFUGET: 0,", download_mon, MON_PAUSED);

API documentation
*****************

| Header file: :file:`include/modem/modem_slm.h`
| Source file: :file:`lib/modem/modem_slm.c`
| Source file: :file:`lib/modem/modem_slm_monitor.c`

.. doxygengroup:: modem_slm
   :project: nrf
   :members:
