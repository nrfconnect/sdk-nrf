.. _nrf_coap_client_sample:

nRF9160: nRF CoAP Client
########################

The nRF CoAP Client sample demonstrates how to receive data from a public CoAP server with an nRF9160 SiP.

Overview
*********

The sample connects to a public CoAP test server, sends periodic GET request for a test resource that is available on the server, and prints the data that is received.

Requirements
************

* The following development board:

  * |nRF9160DK|

* .. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/coap_client`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the kit prints the following information::

       The nRF CoAP client sample started
#. Observe that the kit sends periodic CoAP GET requests after it gets LTE connection.
   The kit sends requests to the configured server (``CONFIG_COAP_SERVER_HOSTNAME``) for a configured resource (``CONFIG_COAP_RESOURCE``).
#. Observe that the kit either prints the reponse data received from the server or indicates a time-out.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * ``drivers/lte_link_control``

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`CoAP <zephyr:networking_reference>`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`
