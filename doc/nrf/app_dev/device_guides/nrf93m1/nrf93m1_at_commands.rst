.. _ug_nrf93m1_at_commands:

AT command interface
####################

.. contents::
   :local:
   :depth: 2

AT commands are the primary control interface for an nRF93M1 module.
Using this method, you can access every feature of the module, including cloud services, location, and power-saving modes.

This page covers how to drive the interface from a host application.
For the command syntax and the full command list, see the `nRF93M1 AT Commands Reference Guide`_.

.. _ug_nrf93m1_at_commands_categories:

Command categories
******************

The command set has three origins, which is useful to know when you are searching for a command.

.. list-table::
   :header-rows: 1

   * - Prefix
     - Origin
     - Examples
   * - ``AT+``
     - 3GPP standardized
     - ``AT+CFUN``, ``AT+CEREG``, ``AT+CPSMS``, ``AT+CEDRXS``, ``AT+IPR``
   * - ``AT%``
     - Nordic proprietary
     - ``AT%BAND``, ``AT%POWD``, ``AT%SYSOFF``, ``AT%MODULECFG``, ``AT%PTWEDRXS``, ``AT%RFTEST``,
       ``AT%DEVICEUUID``, ``AT%NRFCLOUDLOCATION``, ``AT%SKTCREATE``, ``AT%HTTPGET``, ``AT%MQTTPUB``
   * - ``AT``
     - Basic
     - ``AT``, ``ATE``

Standardized commands work similarly to those on other cellular modules, allowing existing host code to be reused with minimal modifications.
Nordic proprietary commands cover module-specific functionality such as power state control, band selection, and the nRF Cloud client.

.. _ug_nrf93m1_at_commands_families:

Command families
****************

The command set does more than control the modem.
The entire modem subsystem is accessible through AT commands, so a host device does not need a networking stack of its own.

.. list-table::
   :header-rows: 1

   * - Family
     - Prefix
     - What it covers
   * - Sockets
     - ``%SKT*``
     - Create, bind, connect, listen, accept, send, and receive TCP and UDP sockets, with receive and error notifications.
       A transparent send mode is available.
   * - HTTP and HTTPS
     - ``%HTTP*``
     - Configuration, URL, GET, range GET, POST, POST from file, read to file, and cancel.
   * - MQTT
     - ``%MQTT*``
     - Configure, open, connect, subscribe, unsubscribe, publish, disconnect, and close, across up to six contexts.
   * - Cloud
     - ``%DEVICEUUID``, ``%REGJWT``, ``%JWT``, ``%NRFCLOUD*``
     - Identity, location, messaging, shadow, FOTA, and Memfault observability.
   * - File system
     - ``%F*``
     - Open, read, write, delete, close, SHA-256 hash, and file system information.
   * - Packet domain
     - ``+CG*``, ``%DNS``, ``%PING``, ``%GDCNT``
     - PDP contexts, attach, DNS, ping, and data counters.
   * - Power
     - ``%SYSOFF``, ``%POWD``, ``%MODULECFG``, ``%MODEMCFG``, ``%ADC``
     - See :ref:`ug_nrf93m1_power_optimization`.
   * - RF and production
     - ``%RFTEST``, ``%COEX``, ``%RFC``, ``%PRODDONE``
     - Non-signalling test, band coexistence, antenna tuner control, production test completion.
   * - Security
     - ``%SSLCFG``
     - TLS configuration.

.. note::
   The `nRF93M1 DK Hardware User Guide`_ tags each command and parameter with the firmware version that supports it, written as ``vx.x.x``.
   For example, a command tagged ``v1.5.x`` is supported on any 1.5 firmware, regardless of the patch number.
   Verify the tags before you rely on a command, at both command and parameter level, because a supported command can still have unsupported parameters

.. _ug_nrf93m1_at_commands_first:

Getting a first response
************************

The fastest way to reach the AT interface is the :ref:`nrf93m1dk_modem_bypass` sample, which puts the modem UART switch in bypass mode and forwards it to a **USB CDC-ACM VCOM** port on the DK.
Program the sample, open the port in the `Serial Terminal app`_ at 115200 baud, and send commands directly.

The sample also handles module power sequencing at startup, and maps **Button 1** to ``POWER_KEY`` and **Button 2** to ``RESET``, in case you need to power cycle or recover the module without reprogramming.

A minimal bring-up sequence is as follows:

.. code-block:: console

   AT
   OK
   AT+CGMR
   <firmware version>
   OK
   AT%BAND=?
   <supported band bitmap>
   OK
   AT+CLAC
   <list of all supported AT commands>
   OK
   AT+CFUN=1
   OK
   AT+CEREG?
   +CEREG: 1,1,"1A2B","01234567",7
   OK

* ``AT+CGMR`` returns the modem firmware version.
  Use it when reporting issues.
* ``AT%BAND=?`` returns the variant and supported band set.
* ``AT+CLAC`` lists every command the running firmware supports, which is the fastest way to check availability without cross-referencing version tags.
* ``AT+CEREG?`` reports network registration status.
  A second parameter of ``1`` or ``5`` means the module is registered.

.. _ug_nrf93m1_at_commands_host:

Sending commands from a Zephyr host
***********************************

Zephyr provides ``modem_chat``, a script-driven AT command handler that manages request and response matching, timeouts, and unsolicited result codes.

Enable it with the following options:

.. code-block:: cfg

   CONFIG_MODEM_MODULES=y
   CONFIG_MODEM_CHAT=y
   CONFIG_MODEM_BACKEND_UART=y

The pattern is to define a chat script of request and expected response pairs, then run it against the UART backend.
See the :ref:`nrf93m1dk_ppp_shell` sample, which uses this mechanism through the Zephyr modem cellular driver, and the Zephyr ``modem_chat`` API documentation.

If you use the PPP model, the cellular modem driver owns the AT channel and runs its own chat scripts during initialization and monitoring.
Do not inject commands onto that channel yourself, because the driver's state machine tracks what it expects to receive.

Instead, enable the :kconfig:option:`CONFIG_MODEM_AT_SHELL` option, which exposes the AT channel through a shell backend that shares the driver's CMUX DLCI properly.
This is what the :ref:`nrf93m1dk_ppp_shell` sample does.

.. _ug_nrf93m1_at_commands_urc:

Handling unsolicited result codes
*********************************

The module reports asynchronous events as unsolicited result codes (URCs).
A host application that only sends commands and waits for ``OK`` will miss them.

Subscribe to the following common events:

.. list-table::
   :header-rows: 1

   * - Event
     - Command to enable
     - Why it matters
   * - Network registration changes
     - ``AT+CEREG=`` with a non-zero level
     - Detect loss of coverage without polling
   * - Incoming SMS
     - ``AT+CNMI``
     - Required if your design uses SMS

Subscribe to all URCs you need during initialization, before you bring the radio up with ``AT+CFUN=1``.
Events that occur during registration are lost if you subscribe afterwards.

Enable verbose errors early as well:

.. code-block:: console

   AT+CMEE=2
   OK

Without it, a failing command returns a bare ``ERROR`` with no indication of the cause.

.. note::
   URCs arrive at any time, including in the middle of a command exchange.
   ``modem_chat`` handles this correctly, unlike a hand-written parser.

.. _ug_nrf93m1_limitations:

Limitations
***********

Baud rate and sleep
   If the module enters a deep sleep state while the UART is above 9600 bps, the host cannot wake it over UART.
   Set ``AT+IPR`` before allowing deep sleep.
   See :ref:`ug_nrf93m1_power_optimization`.

Flow control
   Enable ``RTS`` and ``CTS``.
   Without flow control, high baud rates lose bytes, which usually presents as intermittent parse failures rather than an obvious error.

Blocking on long operations
   Network registration, provisioning, and firmware updates take seconds to minutes.
   Do not use a short fixed timeout for every command.

Test-only commands in production
   The ``%RFTEST`` command is a characterization tool.
   Do not ship it in production firmware, ``%PRODDONE`` must be enabled when production is complete.
