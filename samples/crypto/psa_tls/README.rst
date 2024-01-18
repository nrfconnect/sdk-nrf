.. _crypto_tls:

Crypto: PSA TLS
###############

.. contents::
   :local:
   :depth: 2

The PSA TLS sample shows how to perform a TLS handshake and send encrypted messages with Cortex-M Security Extensions (CMSE) enabled, in both Non-Secure Processing Environment (NSPE) and Secure Processing Environment (SPE) with :ref:`Trusted Firmware-M<ug_tfm>`.

.. note::

   Datagram Transport Layer Security (DTLS) and Pre-shared key (PSK) are currently not supported.

For information about CMSE and the difference between the two environments, see :ref:`app_boards_spe_nspe`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires a `TAP adapter`_ to perform the TLS handshake.
This functionality is currently only supported in Linux.

Overview
********

The sample can act as either a network server or a network client.
By default, the sample is configured to act as a server.

Configuration options
*********************

You can configure the following options to make the sample act as either a server or a client:

.. _CONFIG_PSA_TLS_SAMPLE_TYPE_SERVER:

CONFIG_PSA_TLS_SAMPLE_TYPE_SERVER
   Set to ``y`` to make the sample act as a server.
   When acting as a server, the sample waits for a connection from the client on port 4243.
   After the TCP connection is established, the sample automatically initiates the TLS handshake by waiting for the "ClientHello" message from the client.

.. _CONFIG_PSA_TLS_SAMPLE_TYPE_CLIENT:

CONFIG_PSA_TLS_SAMPLE_TYPE_CLIENT
   Set to ``y`` to make the sample act as a client.
   When acting as a client, the sample tries to connect to the server on port 4243.
   After the TCP connection is established, the sample automatically initiates the TLS handshake by sending the "ClientHello" message.

After a successful TLS handshake, the client and server echo any message received over the secured connection.

Certificates
============

The sample supports certificates signed with either ECDSA or RSA.
By default, the sample is configured to use ECDSA certificates.
Set the ``CONFIG_PSA_TLS_CERTIFICATE_TYPE_RSA`` option to ``y`` to make the sample use RSA certificates.

Certificates when running with CMSE
-----------------------------------

When the sample is compiled for NSPE alongside SPE, that is with CMSE enabled, it stores its TLS certificates and keys in the TF-M Protected Storage.
During the sample initialization, the certificates and keys are fetched from TF-M Protected Storage and kept in non-secure RAM for use during every subsequent TLS handshake.

.. note::
   Currently, applications with CMSE enabled only support ECDSA certificates.
   This is automatically enforced in the configuration files for build targets with CMSE enabled (``*_ns``).

Supported cipher suites
=======================

The sample supports most TLS v1.2 cipher suites, except for the following combinations:

* RSA is not supported in applications with CMSE enabled.
* AES256 is not supported in applications with CMSE enabled that are running on nRF9160.

TAP adapter
===========

The sample uses the :ref:`lib_eth_rtt` library in |NCS| to transfer Ethernet frames to the connected PC.
The PC must parse the incoming RTT data and dispatch these packets to a running TAP network device.
This functionality is called *TAP adapter*.

In Linux
--------

The TAP adapter functionality for Linux is included in the `Ethernet over RTT for Linux`_ executable, named :file:`eth_rtt_link`, located in the :file:`samples/crypto/psa_tls` folder.
You must pass the development kit's SEGGER ID and the TAP IPv4 as parameters when calling the executable.
See the examples in the `Testing`_ section.

When using an nRF5340 development kit, if :file:`eth_rtt_link` cannot start the RTT connection, pass the ``_SEGGER_RTT`` RAM block address as a parameter using ``--rttcbaddr``, as shown in the following example:

.. code-block:: console

   sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.2 --rttcbaddr 0x20002000

You can find the ``_SEGGER_RTT`` RAM address in the :file:`.map` file.


In Windows
----------

There is currently no direct support for TAP adapters in Windows.
However, you can still follow the steps given in the `Testing`_ section using a Linux distribution running inside a virtual machine.

Openssl
=======

Use the openssl command-line interface to perform the peer network operations when testing this sample.

It can act as either client or server with simple commands in a terminal.
For more information, see the `Testing`_ section.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/psa_tls`

.. include:: /includes/build_and_run_ns.txt

.. _crypto_tls_testing:

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

.. tabs::

   .. tab:: Test the sample as a server

      1. Start a terminal emulator like nRF Connect Serial Terminal and connect to the used serial port with the standard UART settings.
         See :ref:`test_and_optimize` for more information.
      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as superuser with your development kit's segger-id and the following IPv4 address as parameters:

         .. code-block:: console

           sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.2

      #. Use ``openssl`` to perform the `client` connection and handshake operation.

         .. code-block:: console

           openssl s_client -connect 192.0.2.1:4243 -cipher ECDHE-ECDSA-AES128-SHA256 -CAfile certs/ecdsa/root_cert.pem

         .. note::

            If the sample has been built with an RSA certificate, use this ``openssl`` command:

            .. code-block:: console

               openssl s_client -connect 192.0.2.1:4243 -cipher AES128-SHA256 -CAfile certs/rsa/root_cert.pem

         For visualizing a list of the available cipher suites for openssl, use the following command:

         .. code-block:: console

           openssl ciphers

      #. Type ``Nordic Semiconductor`` into the ``openssl`` connection session to send ``Nordic Semiconductor`` as an encrypted message to the server.
      #. Check that the TLS sample returns ``Nordic Semiconductor`` in the ``openssl`` session.
      #. Check in the terminal emulator that 21 bytes were successfully received and returned.


   .. tab:: Test the sample as a client

      1. Start a terminal emulator like nRF Connect Serial Terminal and connect to the used serial port with the standard UART settings.
         See :ref:`test_and_optimize` for more information.
      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as superuser with your development kit's segger-id and the following IPv4 address as parameters:

         .. code-block:: console

           sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.1

      #. Use ``openssl`` to perform the `client` connection and handshake operation.

         .. code-block:: console

           openssl s_server -accept 4243 -cipher ECDHE-ECDSA-AES128-SHA256 -cert certs/ecdsa/cert.pem -key certs/ecdsa/cert.key

         .. note::

            If the sample has been built with an RSA certificate, use this ``openssl`` command:

            .. code-block:: console

               openssl s_server -accept 4243 -cipher AES128-SHA256 -cert certs/rsa/cert.pem -key certs/rsa/cert.key

         For visualizing a list of the available cipher suites for openssl, use the following command:

         .. code-block:: console

           openssl ciphers

      #. Type ``Nordic Semiconductor`` into the ``openssl`` connection session to send the string ``Nordic Semiconductor`` as an encrypted message to the server.
      #. Check that the TLS sample returns ``Nordic Semiconductor`` in the ``openssl`` session.
      #. Check in the terminal emulator that 21 bytes were successfully received and returned.

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`:

  * :file:`include/logging/log.h`

* :ref:`zephyr:bsd_sockets_interface`:

  * :file:`net/socket.h`

* :file:`net/net_core.h`
* :file:`net/tls_credentials.h`

It also uses the following TF-M libraries:

* :file:`tfm_ns_interface.h`
* :file:`psa/storage_common.h`
* :file:`psa/protected_storage.h`
