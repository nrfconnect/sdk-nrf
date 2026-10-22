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
     - TLS and DTLS configuration of the security contexts.

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

.. _ug_nrf93m1_at_commands_credentials:

Provisioning TLS credentials
****************************

The module runs TLS and DTLS itself, and it stores the credentials as files in its own file system.
You provision them with AT commands, and you do not need a host-side TLS library.

The module has six security contexts, numbered 0 to 5, that hold the TLS and DTLS settings.
You configure each context with the ``%SSLCFG`` command, which selects the following files:

* ``cacert``, the certificate authority (CA) certificate that is used to verify the server.
* ``clientcert`` and ``clientkey``, the client certificate and its private key, if the server requires client authentication.

A secure connection then selects a context by its ID:

* For sockets, use ``AT%SKTCFG="sslctxid",<fd>,<SSL_ctxID>``, where ``<fd>`` is the socket descriptor that ``%SKTCREATE`` returns.
  The default value ``255`` disables TLS and DTLS.
  A TCP socket uses TLS, and a UDP socket uses DTLS.
* For MQTT, use ``AT%MQTTCFG="sslctxid",<id>,<SSL_ctxID>``, where ``<id>`` is the MQTT context.
  The default value ``255`` disables TLS.
* For HTTP and HTTPS, use ``AT%HTTPCFG="sslctxid",<SSL_ctxID>``.
  The default is context ``0``.
  The ``%HTTPFOTADL`` command also uses a security context, so the same steps apply when you update the modem firmware over HTTPS.
  See :ref:`ug_nrf93m1_updating_modem_fw`.

The ``sslctxid`` options of ``%SKTCFG`` and ``%MQTTCFG`` are available from modem firmware v1.5.1.

The following example provisions the Amazon Root CA 1 certificate, which is the trust anchor of the ``firmware.nrfcloud.com`` and ``www.amazontrust.com`` servers, and uses it for HTTPS requests.
The example works the same way for the CA certificate of your own server.

.. note::
   The file system of the module is small.
   Run the ``AT%FSINFO`` command to see the stored files and the free space before you add a certificate.

1. Get the CA certificate of your server in PEM format, which starts with ``-----BEGIN CERTIFICATE-----``.
   You can download it from the website of the certificate authority, or export it from your web browser.
   Check the size of the file in bytes, because you need it in the next step:

   .. code-block:: console

      wc -c AmazonRootCA1.pem
      1188 AmazonRootCA1.pem

#. Create the file in the file system of the module.
   Mode ``1`` creates the file, or overwrites it if it exists.
   The file name is limited to 62 bytes.

   .. code-block:: console

      AT%FOPEN="amazon_root_ca1.pem",1
      %FOPEN: 1
      OK

#. Write the certificate to the file.
   The module switches to data mode and accepts the number of bytes that you give as the second parameter.
   The third parameter is the time in seconds that the module waits for the data.
   When the module responds with ``CONNECT``, send the content of the PEM file as it is, with exactly that number of bytes, from the terminal or from your host application:

   .. code-block:: console

      AT%FWRITE=1,1188,40
      CONNECT
      <content of the PEM file>
      %FWRITE: 1188,1188
      OK

#. Close the file:

   .. code-block:: console

      AT%FCLOSE=1
      OK

#. Verify that the file was stored correctly by comparing its SHA-256 hash with the hash of the original file:

   .. code-block:: console

      AT%FSHA256="amazon_root_ca1.pem"
      %FSHA256: 2c43952ee9e000ff2acc4e2ed0897c0a72ad5fa72c3d934e81741cbd54f05bd1
      OK

#. Configure the security context:

   a. Select the certificate file with the ``cacert`` option.
   b. Require the module to verify the server with the ``seclevel`` option.
   c. Enable Server Name Indication (SNI) with the ``sni`` option, so that the module sends the name of the server in the handshake.

   Context ``0`` is the default context of the HTTP commands.

   .. code-block:: console

      AT%SSLCFG="cacert",0,"amazon_root_ca1.pem"
      OK
      AT%SSLCFG="seclevel",0,1
      OK
      AT%SSLCFG="sni",0,1
      OK

   The ``seclevel`` option selects the authentication mode of the context.
   The default value ``0`` means no authentication.
   With no authentication, the module does not verify the certificate of the server, so the CA certificate has no effect.
   Set the option to ``1`` to verify the server.
   Set the option to ``2`` to also authenticate the client if the server requests it, as described at the end of this section.
   To read the current settings of all contexts, run ``AT%SSLCFG?``.
   A context that does not exist returns ``%SSL ERROR: 1``.

#. Send an HTTPS request that uses the context.
   The following example downloads the same certificate from the server over HTTPS:

   .. code-block:: console

      AT%HTTPURL=56,60
      CONNECT
      https://www.amazontrust.com/repository/AmazonRootCA1.pem
      OK
      AT%HTTPGET
      OK
      AT%HTTPREAD
      OK
      CONNECT
      %HTTPGET: 0,200,1188
      -----BEGIN CERTIFICATE-----
      MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
      ...
      -----END CERTIFICATE-----
      %HTTPGET: 100,200,1188

The request fails with the following error if the TLS handshake fails.
For example, when the server certificate is not signed by a CA that the context trusts, or when SNI is disabled for a server that requires it, such as ``www.amazontrust.com`` and ``firmware.nrfcloud.com``:

.. code-block:: console

   %HTTP ERROR: 19

If the ``seclevel`` option is ``1`` or ``2`` and the context has no CA certificate, the request fails with ``%HTTP ERROR: 10``.

To use client authentication, complete the following steps:

1. Write the client certificate and the private key to files in the same way as the CA certificate.
#. Select the files with the ``clientcert`` and ``clientkey`` options of ``%SSLCFG``.
#. Set the ``seclevel`` option to ``2``.

To remove a file, open it with ``%FOPEN`` and delete it with ``AT%FDELETE=<filehandle>``.

From modem firmware v1.5.1, the ``sslversion`` option selects the TLS and DTLS protocol version of the context:

* ``3`` for TLS 1.2 and DTLS 1.2.
* ``4`` for TLS 1.2 and TLS 1.3, and DTLS 1.2.
  This is the default.
* ``5`` for TLS 1.3 and DTLS 1.2.

For example, ``AT%SSLCFG="sslversion",0,5`` allows only TLS 1.3 in context ``0``.

.. note::
   The file system of the module has no access control, so anyone with access to the AT interface can read, change, or delete the stored certificates and keys.
   Compare the ``%FSHA256`` hash of a file with a known-good value before you use it.

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
