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

To enable the library, set the :kconfig:option:`CONFIG_NRF_CLOUD` and :kconfig:option:`CONFIG_NRF_PROVISIONING` Kconfig options.

* :kconfig:option:`CONFIG_NRF_PROVISIONING_AUTO_START_ON_INIT` - Starts and performs provisioning on library initialization.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` - Activates scheduled provisioning at a given interval.
  If disabled, the client will only provision when requested manually using the :c:func:`nrf_provisioning_trigger_manually` function.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_VALID_DATE_TIME_TIMEOUT_SECONDS` - Time to wait for valid date-time before starting provisioning.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_INTERVAL_S` - Maximum provisioning interval, in seconds, only valid if :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` is enabled.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SPREAD_S` - Provisioning time spread in seconds, only valid if :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` is enabled.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_INITIAL_BACKOFF` - Initial time for exponential backoff in seconds, only valid if :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` is enabled.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG` - Root CA security tag for the Provisioning Service.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SAVE_CMD_ID` - Saves the latest command ID to storage.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_CUSTOM_AT` - Activates custom AT commands.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_WITH_CERT` - Provisions the root CA certificate to the security tag if the tag is empty.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_RX_BUF_SZ` - Response buffer size.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_TX_BUF_SZ` - Request buffer size.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_JWT_SEC_TAG` - Security tag for private Device Identity key.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S` - Maximum JWT valid lifetime in seconds.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN` - Provides attestation token when the device is unclaimed/unauthorized through the event handler.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_STACK_SIZE` - Stack size for the nRF Provisioning thread.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SHELL` - Enables shell module, which allows you to control the client over UART.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_SETTINGS_STORAGE_PATH` - Sets the path for provisioning settings storage.

Configuration options for HTTP
==============================

* :kconfig:option:`CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME` - HTTP API hostname for the Provisioning Service, default ``provisioning-http.nrfcloud.com``.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_HTTP_PORT` - Port number for the Provisioning Service.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS` - Timeout in milliseconds for HTTP connection of the Provisioning Service.

Configuration options for CoAP
==============================

* :kconfig:option:`CONFIG_NRF_PROVISIONING_COAP_HOSTNAME` - CoAP API hostname for the Provisioning Service, default ``coap.nrfcloud.com``.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_COAP_PORT` - Port number for the Provisioning Service.
* :kconfig:option:`CONFIG_NRF_PROVISIONING_COAP_DTLS_SESSION_CACHE` - Chooses DTLS session cache.

.. _lib_nrf_provisioning_start:

Usage
*****

The usage of the nRF Device provisioning library is described in the following sections.

Initialization
==============

To use the library, you must initialize it.
Call the :c:func:`nrf_provisioning_init` function and pass the following event callback handler to receive events from the library:

.. code-block:: c

   static void nrf_provisioning_callback(const struct nrf_provisioning_callback_data *event)
   {
      /* Handle events received from the library here */
   }

   /* Initialize the provisioning client */
   ret = nrf_provisioning_init(nrf_provisioning_callback);
   if (ret) {
       LOG_ERR("Failed to initialize provisioning client: %d", ret);
       return ret;
   }

Once initialized, provisioning can take place in one of the following two ways:

* Automatically, at a configured interval set by the :kconfig:option:`CONFIG_NRF_PROVISIONING_INTERVAL_S` Kconfig option if :Kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` is enabled.
  You can trigger provisioning asynchronously using the function :c:func:`nrf_provisioning_trigger_manually`.
* Manually, by calling the :c:func:`nrf_provisioning_trigger_manually` function from the application if :Kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` is disabled.

For an example of how to use the library, see the :ref:`nrf_provisioning_sample` sample.

Provisioning
============

During provisioning, the device receives a set of commands from the server.
This requires the device to deactivate LTE (putting the modem into offline mode) to be able to write the commands to the modem's non-volatile memory.
This is because the modem cannot be connected while any data is being written to its storage area.

When the :kconfig:option:`CONFIG_NRF_PROVISIONING_AUTO_START_ON_INIT` Kconfig option is set, the library initializes and starts provisioning according to the configured interval.
When setting this option, you must ensure that it is called when the device has obtained a network connection and the modem is ready to communicate with the server.
The interval is read from the storage settings and can be updated with a provisioning command like any other key-value pair.

During provisioning, the library first tries to establish the transport for communicating with the service.
This procedure involves a (D)TLS handshake where the client establishes the correct server.
The server uses the JWT generated by the device to authenticate the client.
See :ref:`lib_modem_jwt` for more information on client authentication.

The (D)TLS handshake happens twice:

* Before requesting commands.
* After the execution of the commands, to report the results.

After handling any received commands, the results are reported back to the server when either all the commands succeed or when an error occurs.
If an error occurs, the results of all the commands that are successfully executed before the error and the erroneous result are reported back to the server.
All successfully executed commands will be removed from the server-side queue, but if any errors occur, the erroneous command and all the remaining unexecuted commands are removed from the server-side queue.
The log contains more information about the issue.

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
     now: Do provisioning now
     token: Get the attestation token
     uuid: Get device UUID
     interval: Set provisioning interval

The shell commands depend on the library being initialized in the application and the event handler being set.

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
