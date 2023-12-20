.. _lib_nrf_provisioning:

nRF Cloud device provisioning
#############################

.. contents::
   :local:
   :depth: 2

The nRF Device provisioning library enables a device to connect to nRF Cloud Provisioning Service, part of nRF Cloud Security Services.
It abstracts and hides the details of the transport and encoding scheme that are used for the payload.
The current implementation supports the following technologies:

* AT-command based provisioning commands
* Writing key-value pair based settings to the :ref:`settings_api` storage
* TLS-secured HTTP as the communication protocol
* DTLS-secured CoAP as the communication protocol
* Client authentication with JWT token
* CBOR as the data format

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_NRF_PROVISIONING` Kconfig option to ``y``.

* :kconfig:option:`CONFIG_NRF_PROVISIONING_AUTO_INIT` - Initializes the client in the system initialization phase
* :kconfig:option:`CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG` - Root CA security tag for the Provisioning Service
* :kconfig:option:`CONFIG_NRF_PROVISIONING_INTERVAL_S` - Maximum provisioning interval, in seconds
* :kconfig:option:`CONFIG_NRF_PROVISIONING_WITH_CERT` - Provisions the root CA certificate to the security tag if the tag is empty
* :kconfig:option:`CONFIG_NRF_PROVISIONING_RX_BUF_SZ` - Response buffer size
* :kconfig:option:`CONFIG_NRF_PROVISIONING_TX_BUF_SZ` - Request buffer size
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SHELL` - Enables shell module, which allows you to control the client over UART
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SETTINGS_STORAGE_PATH` - Sets the path for provisioning settings storage

Configuration options for HTTP
==============================

* :kconfig:option:`CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME` - HTTP API hostname for the Provisioning Service, default ``provisioning-http.nrfcloud.com``
* :kconfig:option:`CONFIG_NRF_PROVISIONING_HTTP_PORT` - Port number for the Provisioning Service
* :kconfig:option:`CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS` - Timeout in milliseconds for HTTP connection of the Provisioning Service

Configuration options for CoAP
==============================

* :kconfig:option:`CONFIG_NRF_PROVISIONING_COAP_HOSTNAME` - CoAP API hostname for the Provisioning Service, default ``coap.nrfcloud.com``
* :kconfig:option:`CONFIG_NRF_PROVISIONING_COAP_PORT` - Port number for the Provisioning Service
* :kconfig:option:`CONFIG_NRF_PROVISIONING_COAP_DTLS_SESSION_CACHE` - Chooses DTLS session cache

.. _lib_nrf_provisioning_start:

Usage
*****

The usage of the nRF Device provisioning library is described in the following sections.

Initialization
==============

Once initialized, the provisioning client runs on its own in the background.
The provisioning client can be initialized in one of the following ways:

* The application calls :c:func:`nrf_provisioning_init`, which starts the client.
* Set the client to initialize during Zephyr's system initialization phase.
  In this case, it is assumed that a network connection has been established in the same phase.

The function uses the following arguments:

*  A pointer to a :c:struct:`nrf_provisioning_mm_change` structure that holds a callback function to be called when the modem state changes.
*  A pointer to a :c:struct:`nrf_provisioning_dm_change` structure that holds a callback function to be called when the provisioning state changes.

If you provide ``null`` as a callback function address argument, a corresponding default callback is used.
Subsequent calls to the initialization function will only change the callback functions.
This behavior is beneficial when the client has been initialized during the system initialization phase, but the application wants to register its own callback functions afterwards.

Provisioning
============

By default, when provisioning is done after receiving the ``FINISHED`` command, the device is rebooted.
The behavior can be overwritten by providing a unique callback function for the initialization function.

If anything is written to the modem's non-volatile memory, the modem needs to be set in offline mode.
This is because the modem cannot be connected while any data is being written to its storage area.
Once the memory write is complete, the aforementioned callback function must be called again to set the modem to the desired state.
To use the default implementation, ``NULL`` can be passed as an argument to the :c:func:`nrf_provisioning_init` function.
Copy and modify the default callback function as necessary.

The library starts provisioning when it initializes, then according to the configured interval.
The interval is read from the storage settings and can be updated with a provisioning command like any other key-value pair.

During provisioning, the library first tries to establish the transport for communicating with the service.
This procedure involves a (D)TLS handshake where the client establishes the correct server.
The server uses the JWT generated by the device to authenticate the client.
See :ref:`lib_modem_jwt` for more information on client authentication.

The (D)TLS handshake happens twice:

* Before requesting commands.
* After the execution of the commands, to report the results.

If you are using AT commands, the library shuts down the modem for writing data to the modem's non-volatile memory.
Once the memory writes are complete, the connection is re-established to report the results back to the server.
The results are reported back to the server when either all the commands succeed or when an error occurs.
If an error occurs, the results of all the commands that are successfully executed before the error and the erroneous result are reported back to the server.
All successfully executed commands will be removed from the server-side queue, but if any errors occur, the erroneous command and all the remaining unexecuted commands are removed from the server-side queue.
The log contains more information about the issue.

Immediate provisioning can be requested by calling the :c:func:`nrf_provisioning_trigger_manually` function.
Otherwise, the library attempts provisioning according to the set interval.
To trigger immediate provisioning, the library must be initialized first.

The following message sequence chart shows a successful provisioning sequence:

.. msc::
   hscale = "1.5";
   Owner,Server,Device;
   Owner>>Server     [label="Provision: cmd1, cmd2, finished"];
   Server<<Device    [label="Get commands"];
   Server>>Device    [label="Return commands"];
   Device box Device [label="Decode commands"];
   Device box Device [label="Set modem offline"];
   Device box Device [label="Write to non-volatile memory"];
   Device box Device [label="Restore modem state"];
   Server<<Device    [label="cmd1,cmd2, finished succeeded"];

The following message sequence chart shows a failing provisioning sequence:

.. msc::
   hscale = "1.5";
   Owner,Server,Device;
   Owner>>Server     [label="Provision: cmd1, cmd2, cmd3, finished"];
   Server<<Device    [label="Get commands"];
   Server>>Device    [label="Return commands"];
   Device box Device [label="Decode commands"];
   Device box Device [label="Set modem offline"];
   Device box Device [label="cmd1: Write to non-volatile memory"];
   Device box Device [label="cmd2: Fails"];
   Device box Device [label="Restore modem state"];
   Server<<Device    [label="cmd1 success, cmd2 failed"];
   Server>>Server    [label="Empty the command queue"];
   Server>>Owner     [label="cmd2 failed"];

.. _nrf_provisioning_shell:

nRF Provisioning shell
**********************

To test the client, you can enable Zephyr's shell and provisioning command, which allow you to control the client over UART.
The feature is enabled by selecting :kconfig:option:`CONFIG_NRF_PROVISIONING_SHELL`.

.. note::
   The shell is meant for testing.
   Do not enable it in production.

.. code-block:: console

   uart:~$ nrf_provisioning
   nrf_provisioning - nRF Provisioning commands
   Subcommands:
     init   :Start the client
     now    :Do provisioning now
     token  :Get the attestation token
     uuid   :Get device UUID

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`modem_key_mgmt`
* :ref:`lib_rest_client`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`CoAP <zephyr:networking_api>`
* :ref:`CoAP Client <zephyr:coap_client_interface>`

.. _nrf_provisioning_api:

API documentation
*****************

| Header file: :file:`include/net/nrf_provisioning.h`
| Source files: :file:`subsys/net/lib/nrf_provisioning/src/`

.. doxygengroup:: nrf_provisioning
   :project: nrf
   :members:
