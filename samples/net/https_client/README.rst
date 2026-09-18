.. _https_client:

.. ncs-sample::
   :title: HTTPS Client

   The HTTPS Client sample demonstrates a minimal implementation of HTTP communication on a device that connects to either an LTE network using an nRF91 Series device, or to Wi-Fi® using an nRF71 Series or nRF70 Series device.
   It shows how to set up a TLS session towards an HTTPS server and how to send an HTTP request.

   .. |wifi| replace:: Wi-Fi

   .. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first prepares TLS credentials for the HTTPS connection.
For nRF91 Series cellular builds, it provisions a root CA certificate to modem storage using the :ref:`modem_key_mgmt` library.
For Wi-Fi and other non-modem builds, it registers the certificate with Zephyr TLS credentials.
On nRF91 Series devices, modem certificate provisioning must be done before connecting to the cellular network, because certificates can only be provisioned when the device is not connected.

The sample then establishes a connection to the network, sets up the necessary TLS socket options, and connects to an HTTPS server.
It sends an HTTP HEAD request and prints the response code in the terminal.

Obtaining a certificate
=======================

The sample connects to ``www.example.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/net/https_client/cert` folder.
|hex_format|

To connect to other servers, you might need to provision a different certificate.
See :ref:`cert_dwload` for more information.

Using Mbed TLS and TF-M on nRF91 Series DKs
*******************************************

This sample supports using Mbed TLS and Trusted Firmware-M (TF-M).
Instead of offloading the TLS sockets into the modem, you can use the Mbed TLS library from Zephyr.
Using the Zephyr Mbed TLS, you can still use the offloaded sockets.
Mbed TLS offers more configuration options than using the offloaded TLS handling.

When using TF-M and Mbed TLS with PSA crypto, all the crypto operations are run on the secure side on the device.

Configuration
*************

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :file:`prj.conf` - General configuration file for all devices.
* :file:`boards/nrf9151dk_nrf9151_ns.conf` - Configuration file for the nRF9151 DK.
* :file:`boards/nrf9161dk_nrf9161_ns.conf` - Configuration file for the nRF9161 DK.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/native_sim.conf` - Configuration file for the native simulator emulation.
* :file:`boards/nrf7120dk_nrf7120_cpuapp_ns.conf` - Configuration file for the nRF7120 DK.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - Configuration file for the nRF7002 DK.
* :file:`wifi.conf` - Wi-Fi networking configuration, common to the nRF71 Series and nRF70 Series devices.
  Must be added explicitly for Wi-Fi builds.
* :file:`overlay-tfm-nrf91.conf` - Additional configuration for Mbed TLS and TF-M on nRF91 Series DKs.
* :file:`overlay-pdn-nrf91-ipv4.conf` - Additional configuration to force IPv4-only packet data network on nRF91 Series DKs.

Board files under the :file:`boards/` folder are merged automatically for the selected target.
The other configuration files are not board-specific and must be passed with ``EXTRA_CONF_FILE``.

Building and running
********************

.. |sample path| replace:: :file:`samples/net/https_client`

.. include:: /includes/build_and_run_ns.txt

To build the sample with Mbed TLS and TF-M for the nRF91 Series DKs, add the following to your west build command:

.. code-block:: none

   -DEXTRA_CONF_FILE=overlay-tfm-nrf91.conf

The default packet data network (PDN) configuration is dual stack, which will use an IPv6 address if available (and IPv4 if not).

On the nRF91 Series DKs, for testing IPv4 only, you might need to configure the packet data network settings, adding the following to your build command:

.. code-block:: none

   -DEXTRA_CONF_FILE=overlay-pdn-nrf91-ipv4.conf

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset the kit.
#. Observe that the sample starts, provisions certificates, connects to the network and to example.com, and then sends an HTTP HEAD request.
#. Observe that the HTTP HEAD request returns ``HTTP/1.1 200 OK``.

Sample output
=============

.. tabs::

   .. group-tab:: Wi-Fi

      Output for the default configuration (dual stack, IPV4V6) where the network does not support IPv6:

      .. code-block:: console

         HTTPS client sample started
         Bringing network interface up
         Provisioning certificate
         Connecting to the network
         <add your wifi configuration using shell here if not pre-provisioned>
         <inf> wifi_mgmt_ext: Connection requested
         Network connectivity established and IP address assigned
         Looking up example.com
         Resolved 93.184.215.14 (AF_INET)
         Connecting to example.com:443
         Sent 61 bytes
         Received 377 bytes

         >        HTTP/1.1 200 OK

         Finished, closing socket.
         Network connectivity lost
         Disconnected from the network

   .. group-tab:: Cellular

      Output for the default configuration (dual stack, IPV4V6) where the carrier does not support IPv6:

      .. code-block:: console

         HTTPS client sample started
         Bringing network interface up
         Provisioning certificate
         Certificate mismatch
         Provisioning certificate to the modem
         Connecting to the network
         +CGEV: EXCE STATUS 0
         +CEREG: 2,"8169","0149FB00",7
         +CSCON: 1
         +CGEV: ME PDN ACT 0
         +CEREG: 1,"8169","0149FB00",7,,,"11100000","11100000"
         Network connectivity established and IP address assigned
         Looking up example.com
         Resolved 93.184.215.14 (AF_INET)
         Connecting to example.com:443
         Sent 61 bytes
         Received 377 bytes

         >        HTTP/1.1 200 OK

         Finished, closing socket.
         +CEREG: 0
         +CGEV: ME DETACH
         +CSCON: 0
         Network connectivity lost
         Disconnected from the network

      Output for the default configuration, where the carrier does support IPv6:

      .. code-block:: console

         HTTPS client sample started
         Bringing network interface up
         Provisioning certificate
         Certificate match
         Connecting to the network
         +CGEV: EXCE STATUS 0
         +CEREG: 2,"8169","0149FB00",7
         +CSCON: 1
         +CGEV: ME PDN ACT 0
         +CEREG: 1,"8169","0149FB00",7,,,"11100000","11100000"
         Network connectivity established and IP address assigned
         Looking up example.com
         +CGEV: IPV6 0
         Resolved 2606:2800:21f:cb07:6820:80da:af6b:8b2c (AF_INET6)
         Connecting to example.com:443
         Sent 61 bytes
         Received 377 bytes

         >        HTTP/1.1 200 OK

         Finished, closing socket.
         +CEREG: 0
         +CGEV: ME DETACH
         +CSCON: 0
         Network connectivity lost
         Disconnected from the net

      Output where you override the default packet data network (PDN) configuration to IPv4 only, using the :file:`overlay-pdn-nrf91-ipv4.conf` configuration file:

      .. code-block:: console

         HTTPS client sample started
         Bringing network interface up
         Provisioning certificate
         Certificate match
         Connecting to the network
         +CGEV: EXCE STATUS 0
         +CEREG: 2,"8169","0149FB00",7
         +CSCON: 1
         +CGEV: ME PDN ACT 0
         +CEREG: 1,"8169","0149FB00",7,,,"11100000","11100000"
         Network connectivity established and IP address assigned
         Looking up example.com
         Resolved 93.184.215.14 (AF_INET)
         Connecting to example.com:443
         Sent 61 bytes
         Received 377 bytes

         >        HTTP/1.1 200 OK

         Finished, closing socket.
         +CEREG: 0
         +CGEV: ME DETACH
         +CSCON: 0
         Network connectivity lost
         Disconnected from the network

Troubleshooting
===============

If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor app`_ documentation to learn how to capture modem traces to debug network traffic in Wireshark.
This sample enables modem traces by default.

If you have issues with connectivity on nRF71 Series or nRF70 Series devices, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic in order to debug connectivity issues.
Also verify that the Wi-Fi credentials configured for your device match your access point, as described in the `Configuration`_ section on this page.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`
* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
* :ref:`nrfxlib:nrf_modem`
* :ref:`Trusted Firmware-M <ug_tfm>`

This sample also offers a possibility to use the TF-M module that is at :file:`modules/tee/tfm/` in the |NCS| folder structure.
