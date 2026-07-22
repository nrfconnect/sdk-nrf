.. _nrf_coap_client_sample:
.. _net_coap_client_sample:

CoAP Client
###########

.. contents::
   :local:
   :depth: 2

This sample demonstrates the communication between a public CoAP server and a CoAP client application that is running on a Nordic Semiconductor SoC that enables IP networking through cellular or Wi-Fi® connectivity.

.. |wifi| replace:: Wi-Fi

.. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a public CoAP server IP address or URL available on the Internet.

.. include:: /includes/tfm.txt

Overview
********

The CoAP Client sample performs the following actions:

#. Connect to the configured public CoAP test server.
#. Send a periodic GET request for a test resource that is available on the server.
#. Display the received data about the resource on a terminal emulator.

The public CoAP server used in this sample is Californium CoAP server (``coap://californium.eclipseprojects.io:5683``).
This server runs Eclipse Californium, which is an open source implementation of the CoAP protocol that is targeted at the development and testing of IoT applications.

This sample uses the resource **obs** (Californium observable resource) in the communication between the CoAP client and the public CoAP server.
The communication follows the standard request/response pattern and is based on the change in the state of the value of the resource.
The sample queries one resource at a time.

Selecting the server and resource
==================================

The server and resource to query are configured with Kconfig options and default to the Californium test server:

* :option:`CONFIG_COAP_SAMPLE_SERVER_HOSTNAME` and :option:`CONFIG_COAP_SAMPLE_SERVER_PORT`- The CoAP server to connect to.
* :option:`CONFIG_COAP_SAMPLE_RESOURCE`- The resource to query on that server.

.. _coap_client_sample_mtls:

Mutual DTLS (client certificate authentication)
===============================================

The sample can optionally use DTLS to secure the CoAP communication (CoAPS), with client authentication (mutual TLS).

.. note::
   This functionality is only supported on Wi-Fi boards and not on cellular boards.

Enable the :option:`CONFIG_COAP_SAMPLE_DTLS` option to provision a CA certificate, a client certificate, and a client private key, and to connect over CoAPS using mutual X.509 authentication.
Set :option:`CONFIG_COAP_SAMPLE_CA_CERT_FILE`, :option:`CONFIG_COAP_SAMPLE_CLIENT_CERT_FILE`, and :option:`CONFIG_COAP_SAMPLE_CLIENT_KEY_FILE` to the CA certificate, client certificate, and client private key to provision, matching what the server you connect to expects.

The sample includes an example CA trust chain and client certificate and private key under :file:`cert/`, for use against the `Eclipse Californium`_ CoAP interoperability server:

* :file:`cert/cf-ca.pem` — CA trust chain, used to validate the server certificate
* :file:`cert/cf-client.pem` — client certificate
* :file:`cert/cf-client-key.pem` — client private key (EC P-256)

The :file:`wifi-dtls.conf` extra-conf file configures the sample with mutual X.509 authentication and the cipher suite needed for the Californium interop server.

Wi-Fi
=====

On Wi-Fi boards, use the :file:`wifi.conf` extra-conf file, using the ``coap_client_EXTRA_CONF_FILE`` sysbuild variable.
To perform mutual DTLS (CoAPS) with the Californium interoperability server (see :ref:`Mutual DTLS (client certificate authentication) <coap_client_sample_mtls>`), add the :file:`wifi-dtls.conf` extra-conf file on top of :file:`wifi.conf`.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/net/coap_client/Kconfig`):

.. options-from-kconfig::
   :show-type:

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/net/coap_client`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset the kit.
#. Observe that the following output is displayed in the terminal::

       The CoAP client sample started
#. Observe that the discovered IP address of the public CoAP server is displayed on the terminal emulator.
#. Observe that your DK sends periodic CoAP GET requests to the configured server for a configured resource after it gets LTE connection.
#. Observe that the sample either displays the response data received from the server or indicates a timeout on the terminal.
   For more information on the response codes, see `COAP response codes`_.

Sample output
=============

The sample displays the data in the following format:

.. code-block:: console

   CoAP GET request sent sent to californium.eclipseprojects.io, resource: obs
   CoAP response: code: 0x45, payload: 15:29:45

Instead of displaying every single CoAP frame content, the sample displays only the essential data.
For the above sample output, the information displayed on the terminal conveys the following:

* ``code:0x45`` -  CoAP response code (2.05 - Content), which is constant across responses
* ``payload: 15:39:40`` - the actual message payload (current time in UTC format) from the resource that is queried in this sample

References
**********

`RFC 7252 - The Constrained Application Protocol`_

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :ref:`CoAP client <zephyr:coap_client_interface>`
* :ref:`CoAP <zephyr:coap_sock_interface>`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
