.. _crypto_tls:

Crypto: PSA TLS
###############

.. contents::
   :local:
   :depth: 2

The PSA TLS sample shows how to perform a TLS handshake and send encrypted messages in both a secure and a non-secure domain with :ref:`Trusted Firmware-M<ug_tfm>`.

.. note::

   * Datagram Transport Layer Security (DTLS) and Pre-shared key (PSK) are currently not supported.

Overview
********

The sample can act as either a network server or a network client.
By default, the sample is configured to act as a server.

You can configure the following options to make the sample act as either a server or a client:

* .. option:: CONFIG_PSA_TLS_SAMPLE_TYPE_SERVER

     Set to ``y`` to make the sample act as a server.
     When acting as a server, the sample waits for a connection from the client on port 4243.
     After the TCP connection is established, the sample automatically initiates the TLS handshake by waiting for the *ClientHello* message from the client.

* .. option:: CONFIG_PSA_TLS_SAMPLE_TYPE_CLIENT

     Set to ``y`` to make the sample act as a client.
     When acting as a client, the sample tries to connect to the server on port 4243.
     After the TCP connection is established, the sample automatically initiates the TLS handshake by sending the *ClientHello* message.

After a successful TLS handshake, the client and server echo any message received over the secured connection.


Certificates
============

The sample supports certificates signed with either ECDSA or RSA.
By default, the sample is configured to use ECDSA certificates.
You can configure the following option to ``y`` to make the sample use RSA certificates:

* .. option:: CONFIG_PSA_TLS_CERTIFICATE_TYPE_RSA


Certificates when running in a non-secure domain
------------------------------------------------

When the sample is compiled for a non-secure domain, it stores its TLS certificates and keys in the TF-M Protected Storage.
During the sample initialization, the certificates and keys are fetched from TF-M Protected Storage and kept in non-secure RAM for use during every subsequent TLS handshake.

.. note::
   Currently, non-secure applications only support ECDSA certificates.
   This is automatically enforced in the configuration files for non-secure builds.


Supported cipher suites
=======================

The sample supports most TLS v1.2 cipher suites, except for the following combinations:

* RSA is not supported in non-secure applications.
* AES256 is not supported in non-secure applications running on the nRF9160.


TAP adapter
===========

The sample uses the :ref:`lib_eth_rtt` library in |NCS| to transfer Ethernet frames to the connected PC.
The PC must parse the incoming RTT data and dispatch these packets to a running TAP network device.
This functionality is called *TAP adapter*.


In Linux
--------

The TAP adapter functionality for Linux is included in the `Ethernet over RTT for Linux`_ executable, named :file:`eth_rtt_link`, located in the :file:`samples/crypto/psa_tls` folder.
You have to pass the development kit's Segger-ID and the TAP IPv4 as parameters when calling the executable.
See the examples in the `Testing`_ section below.

When using an nRF5340 development kit, if :file:`eth_rtt_link` is not able to start the RTT connection, pass the ``_SEGGER_RTT`` RAM block address as a parameter using ``--rttcbaddr``, as shown in the following example:

.. code-block:: console

      sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.2 --rttcbaddr 0x20002000

You can find the ``_SEGGER_RTT`` RAM address in the ``.map`` file.


In Windows
----------

There is currently no direct support for TAP adapters in Windows.
However, you can still follow the steps given in the `Testing`_ section using a Linux distribution running inside a virtual machine.


Openssl
=======

You can use the openssl command-line interface to perform the peer network operations when testing this sample.

It can act as either client or server with simple commands in a terminal.
For more information, see the `Testing`_ section below.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_ns, nrf9160dk_nrf9160_ns

The sample requires a `TAP adapter`_ to perform the TLS handshake.
This functionality is currently only supported in Linux.


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/psa_tls`

.. include:: /includes/config_build_and_run.txt


.. _crypto_tls_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

.. tabs::

   .. tab:: Test the sample as a server

      1. Start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

         * Baud rate: 115.200
         * 8 data bits
         * 1 stop bit
         * No parity
         * HW flow control: None

      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as superuser with your development kit's segger-id and the following ipv4 address as parameters:

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

      1. Start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

         * Baud rate: 115.200
         * 8 data bits
         * 1 stop bit
         * No parity
         * HW flow control: None

      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as superuser with your development kit's segger-id and the following ipv4 address as parameters:

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

  * ``include/logging/log.h``

* :ref:`zephyr:bsd_sockets_interface`:

  * ``net/socket.h``

* ``net/net_core.h``
* ``net/tls_credentials.h``

This sample also uses the following TF-M libraries:

* ``tfm_ns_interface.h``
* ``psa/storage_common.h``
* ``psa/protected_storage.h``
