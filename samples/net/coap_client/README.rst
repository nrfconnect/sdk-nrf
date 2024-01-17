.. _nrf_coap_client_sample:
.. _net_coap_client_sample:

CoAP Client
###########

.. contents::
   :local:
   :depth: 2

The CoAP Client sample demonstrates the communication between a public CoAP server and a CoAP client application that is running on a Nordic SoC that enables IP networking through cellular or Wi-Fi connectivity.

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

#. Connect to the configured public CoAP test server (specified by the Kconfig option :ref:`CONFIG_COAP_SERVER_HOSTNAME <CONFIG_COAP_SERVER_HOSTNAME>`).
#. Send periodic GET request for a test resource (specified by the Kconfig option :ref:`CONFIG_COAP_RESOURCE <CONFIG_COAP_RESOURCE>`) that is available on the server.
#. Display the received data about the resource on a terminal emulator.

The public CoAP server used in this sample is Californium CoAP server (``coap://californium.eclipseprojects.io:5683``).
This server runs Eclipse Californium, which is an open source implementation of the CoAP protocol that is targeted at the development and testing of IoT applications.

This sample uses the resource **obs** (Californium observable resource) in the communication between the CoAP client and the public CoAP server.
The communication follows the standard request/response pattern and is based on the change in the state of the value of the resource.
The sample queries one resource at a time.
To configure other resources, use the Kconfig option :ref:`CONFIG_COAP_RESOURCE <CONFIG_COAP_RESOURCE>`.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options in the :file:`coap_client/prj.conf` file:

.. _CONFIG_COAP_RESOURCE:

CONFIG_COAP_RESOURCE - CoAP resource configuration
   This option sets the CoAP resource. Default is Californium observable resource.

.. _CONFIG_COAP_SERVER_HOSTNAME:

CONFIG_COAP_SERVER_HOSTNAME - CoAP server hostname
   This option sets the CoAP server hostname. Default is ``californium.eclipseprojects.io``.

.. _CONFIG_COAP_SERVER_PORT:

CONFIG_COAP_SERVER_PORT - CoAP server port
   This option sets the port for the CoAP server. Default is ``5683``.

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
