.. _https_client:

nRF9160: HTTPS Client
#####################

.. contents::
   :local:
   :depth: 2

The HTTPS Client sample demonstrates a minimal implementation of HTTP communication.
It shows how to set up a TLS session towards an HTTPS server and how to send an HTTP request.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Overview
********

The sample first initializes the :ref:`nrfxlib:nrf_modem` and AT communications.
Next, it provisions a root CA certificate to the modem using the :ref:`modem_key_mgmt` library.
Provisioning must be done before connecting to the LTE network, because the certificates can only be provisioned when the device is not connected.

The sample then establishes a connection to the LTE network, sets up the necessary TLS socket options, and connects to an HTTPS server.
It sends an HTTP HEAD request and prints the response code in the terminal.

Obtaining a certificate
=======================

The sample connects to ``www.google.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/nrf9160/https_client/cert` folder.

To connect to other servers, you might need to provision a different certificate.
See :ref:`cert_dwload` for more information.


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/https_client`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the sample starts, provisions certificates, connects to the LTE network and to google.com, and then sends an HTTP HEAD request.
#. Observe that the HTTP HEAD request returns ``HTTP/1.1 200 OK``.

Sample Output
=============

The sample shows the following output:

.. code-block:: console

   HTTPS client sample started
   Provisioning certificate
   Waiting for network.. OK
   Connecting to google.com
   Sent 64 bytes
   Received 903 bytes

   >        HTTP/1.1 200 OK

   Finished, closing socket.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`at_cmd_readme`
* :ref:`at_notif_readme`
* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
