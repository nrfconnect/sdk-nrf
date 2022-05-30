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

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first initializes the :ref:`nrfxlib:nrf_modem` and AT communications.
Next, it provisions a root CA certificate to the modem using the :ref:`modem_key_mgmt` library.
Provisioning must be done before connecting to the LTE network, because the certificates can only be provisioned when the device is not connected.

The sample then establishes a connection to the LTE network, sets up the necessary TLS socket options, and connects to an HTTPS server.
It sends an HTTP HEAD request and prints the response code in the terminal.

Obtaining a certificate
=======================

The sample connects to ``www.example.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/nrf9160/https_client/cert` folder.

To connect to other servers, you might need to provision a different certificate.
See :ref:`cert_dwload` for more information.

Using Mbed TLS and TF-M
***********************

This sample supports using Mbed TLS and Trusted Firmware-M (TF-M).
Instead of offloading the TLS sockets into the modem, you can use the Mbed TLS library from Zephyr.
Using the Zephyr Mbed TLS, you can still use the offloaded sockets.
Mbed TLS offers more configuration options than using the offloaded TLS handling.

When using TF-M and Mbed TLS with PSA crypto, all the crypto operations are run on the secure side on the device.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/https_client`

.. include:: /includes/build_and_run_ns.txt

To build the sample with Mbed TLS and TF-M, add the following to your west build command:

.. code-block:: none

   -DOVERLAY_CONFIG=overlay-tfm_mbedtls.conf

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the sample starts, provisions certificates, connects to the LTE network and to example.com, and then sends an HTTP HEAD request.
#. Observe that the HTTP HEAD request returns ``HTTP/1.1 200 OK``.

Sample output
=============

The sample shows the following output:

.. code-block:: console

   HTTPS client sample started
   Provisioning certificate
   Waiting for network.. OK
   Connecting to example.com
   Sent 64 bytes
   Received 903 bytes

   >        HTTP/1.1 200 OK

   Finished, closing socket.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

This sample also offers a possibility to use the TF-M module that is at :file:`modules/tee/tfm/` in the |NCS| folder structure.
