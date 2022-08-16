.. _lib_at_host:

AT Host
#######

.. contents::
   :local:
   :depth: 2

The AT Host library exposes the AT command interface of the cellular modem for supported devices on an external serial interface.

Configuration
*************

The library is enabled and configured entirely using the Kconfig system.
The external serial port to use, the termination character, and other parameters are configurable.

Specifically, if it is desirable for the client to use SMS functionality, the termination character should not be set to ``<CR>``, since this character is used inside the ``AT+CMGS`` command.

The various configuration options used in the library are listed below:

* :kconfig:option:`CONFIG_AT_HOST_LIBRARY` - Enables the AT Host library
* :kconfig:option:`CONFIG_AT_HOST_UART_INIT_TIMEOUT` - Sets the timeout value to wait for a valid UART line on initialization
* :kconfig:option:`CONFIG_AT_HOST_CMD_MAX_LEN` - Sets the maximum length for an AT command
* :kconfig:option:`CONFIG_AT_HOST_THREAD_PRIO` - Sets the priority level for the AT host work queue thread

The library will use ``uart0`` by default.
However, ``ncs,at-host-uart`` choice in devicetree can be used to change the UART device in the following way:

.. code-block:: devicetree

   / {
      chosen {
         ncs,at-host-uart = &uart1;
      };
   };

Configuration options for setting the termination character:

* :kconfig:option:`CONFIG_NULL_TERMINATION` - Enables ``<NULL>`` as the termination character
* :kconfig:option:`CONFIG_CR_TERMINATION` - Enables ``<CR>`` as the termination character
* :kconfig:option:`CONFIG_LF_TERMINATION` - Enables ``<LF>`` as the termination character
* :kconfig:option:`CONFIG_CR_LF_TERMINATION` - Enables ``<CR+LF>`` as the termination character

Usage
*****

The library has no globally available members, and thus no header file.
If enabled, the library will initialize with the system.
Any input on the configured serial port will be forwarded to the modem upon receipt of the configured termination character(s).
Only one command can be processed at any time, and no data must be sent to the serial port while the command is being processed.
