.. _nrf_coap_client_sample:

nRF9160: nRF CoAP Client
########################

.. contents::
   :local:
   :depth: 2

The nRF CoAP Client sample demonstrates the communication between a public CoAP server and an nRF9160 SiP that acts as the CoAP client.

Overview
********

The nRF CoAP Client sample performs the following actions:

#. Connect to the configured public CoAP test server (specified by the Kconfig parameter ``CONFIG_COAP_SERVER_HOSTNAME``).
#. Send periodic GET request for a test resource (specified by the Kconfig parameter ``CONFIG_COAP_RESOURCE``) that is available on the server.
#. Display the received data about the resource on a terminal emulator.

The public CoAP server used in this sample is Californium CoAP server (``coap://californium.eclipseprojects.io:5683``).
This server runs Eclipse Californium, which is an open source implementation of the CoAP protocol that is targeted at the development and testing of IoT applications.
An nRF9160 DK is used as the CoAP client.

This sample uses the resource **obs** (Californium observable resource) in the communication between the CoAP client and the public CoAP server.
The communication follows the standard request/response pattern and is based on the change in the state of the value of the resource.
The sample queries one resource at a time.
Other resources can be configured using the Kconfig option ``CONFIG_COAP_RESOURCE``.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

The sample also requires a public CoAP server IP address or URL available on the internet.

.. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/coap_client`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample and all prerequisites to the development kit, test it by performing the following steps:

1. Connect your nRF9160 DK to the PC using a USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the following information is displayed::

       The nRF CoAP client sample started
#. Observe that the discovered IP address of the public CoAP server is displayed on the terminal emulator.
#. Observe that the nRF9160 DK sends periodic CoAP GET requests to the configured server for a configured resource after it gets LTE connection.
#. Observe that the sample either displays the response data received from the server or indicates a time-out on the terminal.
   For more information on the response codes, see `COAP response codes`_.



Sample Output
=============

The sample displays the data in the following format:

.. code-block:: console

   CoAP request sent: token 0x9772
   CoAP response: code: 0x45, token 0x9772, payload: 15:39:40

Instead of displaying every single CoAP frame content, the sample displays only the essential data.
For the above sample output, the information displayed on the terminal conveys the following:

* ``code:0x45`` -  CoAP response code (2.05 - Content), which is constant across responses
* ``token 0x9772`` - CoAP token, which is unique per request/response pair
* ``payload: 15:39:40`` - the actual message payload (current time in UTC format) from the resource that is queried in this sample


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`CoAP <zephyr:networking_api>`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`

References
**********

`RFC 7252 - The Constrained Application Protocol`_
