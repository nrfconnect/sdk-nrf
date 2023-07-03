.. _https_client:

HTTPS Client
############

.. contents::
   :local:
   :depth: 2

The HTTPS Client sample demonstrates a minimal implementation of HTTP communication.
It shows how to set up a TLS session towards an HTTPS server and how to send an HTTP request.
The sample connects to an LTE network using an nRF91 Series DK or to Wi-Fi using the nRF7002 DK through the connection manager API.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first initializes the :ref:`nrfxlib:nrf_modem` and AT communications.
Next, it provisions a root CA certificate to the modem using the :ref:`modem_key_mgmt` library.
When using an nRF91 Series device, the provisioning must be done before connecting to the LTE network, because the certificates can only be provisioned when the device is not connected.

The sample then establishes a connection to the network, sets up the necessary TLS socket options, and connects to an HTTPS server.
It sends an HTTP HEAD request and prints the response code in the terminal.

Obtaining a certificate
=======================

The sample connects to ``www.example.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/net/https_client/cert` folder.

To connect to other servers, you might need to provision a different certificate.
See :ref:`cert_dwload` for more information.

Using Mbed TLS and TF-M
***********************

This sample supports using Mbed TLS and Trusted Firmware-M (TF-M).
Instead of offloading the TLS sockets into the modem, you can use the Mbed TLS library from Zephyr.
Using the Zephyr Mbed TLS, you can still use the offloaded sockets.
Mbed TLS offers more configuration options than using the offloaded TLS handling.

When using TF-M and Mbed TLS with PSA crypto, all the crypto operations are run on the secure side on the device.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/net/https_client`

.. include:: /includes/build_and_run_ns.txt

To build the sample with Mbed TLS and TF-M, add the following to your west build command:

.. code-block:: none

   -DOVERLAY_CONFIG=overlay-tfm_mbedtls.conf

The default packet data network (PDN) configuration is dual stack, which will use an IPv6 address if available (and IPv4 if not).

For testing IPv4 only, you might need to configure the packet data network settings, adding the following to your build command:

.. code-block:: none

   -DOVERLAY_CONFIG=overlay-pdn_ipv4.conf

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your DK.
#. Open a terminal emulator and observe that the sample starts, provisions certificates, connects to the network and to example.com, and then sends an HTTP HEAD request.
#. Observe that the HTTP HEAD request returns ``HTTP/1.1 200 OK``.

Sample output
=============

Output for the default configuration (dual stack, IPV4V6) where the carrier does not support IPv6:

.. code-block:: console

   HTTPS client sample started
   Certificate match
   Waiting for network.. PDP context 0 activated
   OK
   Waiting for IPv6..
   IPv6 not available
   Looking up example.com
   Resolved 93.184.216.34 (AF_INET)
   Connecting to example.com:443
   Sent 61 bytes
   Received 370 bytes

   >        HTTP/1.1 200 OK

   Finished, closing socket.
   PDP context 0 deactivated

Output for the default configuration, where the carrier does support IPv6:

.. code-block:: console

   HTTPS client sample started
   Bringing network interface up
   Provisioning certificate
   Certificate match
   Connecting to the network
   Looking up example.com
   Resolved 2606:2800:220:1:248:1893:25c8:1946 (AF_INET6)
   Connecting to example.com:443
   Sent 61 bytes
   Received 370 bytes

   >        HTTP/1.1 200 OK

   Finished, closing socket.
   PDP context 0 deactivated

Output where you override the default packet data network (PDN) configuration to IPv4 only, via the `overlay-pdn_ipv4.conf` overlay:

.. code-block:: console

   HTTPS client sample started
   Bringing network interface up
   Provisioning certificate
   Looking up example.com
   Resolved 93.184.216.34 (AF_INET)
   Connecting to example.com:443
   Sent 61 bytes
   Received 370 bytes

   >        HTTP/1.1 200 OK

   Finished, closing socket.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`
* :ref:`pdn_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

This sample also offers a possibility to use the TF-M module that is at :file:`modules/tee/tfm/` in the |NCS| folder structure.
