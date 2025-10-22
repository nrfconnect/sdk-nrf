.. _crypto_tls:

Crypto: PSA TLS
###############

.. contents::
   :local:
   :depth: 2

The Platform Security Architecture Transport Layer Security (PSA TLS) sample shows how to perform a TLS handshake and send encrypted messages with Cortex-M Security Extensions (CMSE) enabled, in both Non-Secure Processing Environment (NSPE) and Secure Processing Environment (SPE) with :ref:`Trusted Firmware-M<ug_tfm>`.

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

TLS version support
===================

The sample supports both TLS v1.2 and TLS v1.3.
See the following table for the support overview for each compatible board target.
The table mirrors the test setup used for TLS verification.

.. tabs::

   .. tab:: TLS v1.2

      .. list-table::
         :header-rows: 1
         :widths: auto

         * - Board target
           - Backend
           - CMSE supported
           - DTLS supported
           - Limitations
           - Comments
         * - ``nrf52840dk/nrf52840``
             ``nrf9160dk/nrf9160``
             ``nrf9151dk/nrf9151``
           - :ref:`Mbed TLS legacy nrf_cc3xx backend <nrf_security_backends_cc3xx>`
           - No
           - No
           - AES256, AES-GCM, SHA-512
           -
         * - ``nrf52840dk/nrf52840``
             ``nrf9160dk/nrf9160``
             ``nrf9151dk/nrf9151``
           - :ref:`Mbed TLS legacy oberon backend<nrf_security_backends_oberon>`
           - No
           - No
           -
           -
         * - ``nrf52840dk/nrf52840``
           - :ref:`PSA Crypto nrf_cc3xx driver<nrf_security_drivers_cc3xx>`
           - No (secure only)
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf9160dk/nrf9160``
             ``nrf9151dk/nrf9151``
           - :ref:`PSA Crypto nrf_cc3xx driver<nrf_security_drivers_cc3xx>`
           - Yes
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf5340dk/nrf5340/cpuapp``
           - :ref:`Mbed TLS legacy nrf_cc3xx backend <nrf_security_backends_cc3xx>`
           - No
           - No
           -
           -
         * - ``nrf5340dk/nrf5340/cpuapp``
           - :ref:`Mbed TLS legacy nrf_oberon backend<nrf_security_backends_oberon>`
           - No
           - No
           -
           -
         * - ``nrf5340dk/nrf5340/cpuapp``
           - :ref:`PSA Crypto nrf_cc3xx driver<nrf_security_drivers_cc3xx>`
           - Yes
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54l15dk/nrf54l15/cpuapp``
           - :ref:`PSA Crypto CRACEN driver<nrf_security_drivers_cracen>`
           - Yes
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54l15dk/nrf54l10/cpuapp``
           - :ref:`PSA Crypto CRACEN driver<nrf_security_drivers_cracen>`
           - No
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54lm20dk/nrf54lm20a/cpuapp``
           - :ref:`PSA Crypto CRACEN driver<nrf_security_drivers_cracen>`
           - No
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54h20dk/nrf54h20/cpuapp``
             ``nrf54h20dk/nrf54h20/cpurad``
           - SSF Crypto (`CONFIG_PSA_SSF_CRYPTO_CLIENT`_)
           - No
           - Yes
           - RSA not supported
           -

   .. tab:: TLS v1.3

      .. list-table::
         :header-rows: 1
         :widths: auto

         * - Board target
           - Backend
           - CMSE supported
           - DTLS supported
           - Limitations
           - Comments
         * - ``nrf54l15dk/nrf54l15/cpuapp``
           - :ref:`PSA Crypto CRACEN driver<nrf_security_drivers_cracen>`
           - Yes
           - No
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54l15dk/nrf54l10/cpuapp``
           - :ref:`PSA Crypto CRACEN driver<nrf_security_drivers_cracen>`
           - No
           - No
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54lm20dk/nrf54lm20a/cpuapp``
           - :ref:`PSA Crypto CRACEN driver<nrf_security_drivers_cracen>`
           - No
           - Yes
           - RSA not supported
           - Also supports :ref:`PSA Crypto nrf_oberon driver<nrf_security_drivers_oberon>`
         * - ``nrf54h20dk/nrf54h20/cpuapp``
             ``nrf54h20dk/nrf54h20/cpurad``
           - SSF Crypto (`CONFIG_PSA_SSF_CRYPTO_CLIENT`_)
           - No
           - No
           - RSA not supported
           -

.. _crypto_tls_suites:

Supported cipher suites
=======================

See the following tabs for the list of supported and verified cipher suites for each TLS version.
Cipher suites not listed may still work but have not been verified.

.. tabs::

   .. tab:: TLS v1.2

      Supported rsa_ciphers suites:

      * AES128-GCM-SHA256
      * AES128-SHA
      * AES128-SHA256
      * AES256-GCM-SHA384
      * AES256-SHA
      * AES256-SHA256
      * DHE-RSA-AES128-CCM
      * DHE-RSA-AES128-GCM-SHA256
      * DHE-RSA-AES128-SHA
      * DHE-RSA-AES128-SHA256
      * DHE-RSA-AES256-GCM-SHA384
      * DHE-RSA-AES256-SHA
      * DHE-RSA-AES256-SHA256
      * DHE-RSA-CHACHA20-POLY1305
      * ECDHE-RSA-AES128-GCM-SHA256
      * ECDHE-RSA-AES128-SHA
      * ECDHE-RSA-AES128-SHA256
      * ECDHE-RSA-AES256-GCM-SHA384
      * ECDHE-RSA-AES256-SHA
      * ECDHE-RSA-AES256-SHA384
      * ECDHE-RSA-CHACHA20-POLY1305

      Supported rsa_ciphers_cc310_legacy suites (AES256 and GCM removed):

      * AES128-SHA
      * AES128-SHA256
      * DHE-RSA-AES128-CCM
      * DHE-RSA-AES128-SHA
      * DHE-RSA-AES128-SHA256
      * DHE-RSA-CHACHA20-POLY1305
      * ECDHE-RSA-AES128-SHA
      * ECDHE-RSA-AES128-SHA256
      * ECDHE-RSA-CHACHA20-POLY1305

      Supported ecdsa_ciphers suites:

      * ECDHE-ECDSA-AES128-GCM-SHA256
      * ECDHE-ECDSA-AES128-SHA
      * ECDHE-ECDSA-AES128-SHA256
      * ECDHE-ECDSA-AES128-CCM8
      * ECDHE-ECDSA-AES128-CCM
      * ECDHE-ECDSA-AES256-GCM-SHA384
      * ECDHE-ECDSA-AES256-SHA
      * ECDHE-ECDSA-AES256-SHA384
      * ECDHE-ECDSA-AES256-CCM8
      * ECDHE-ECDSA-AES256-CCM
      * ECDHE-ECDSA-CHACHA20-POLY1305
      * ECDHE-PSK-AES128-CBC-SHA256
      * ECDHE-PSK-AES256-CBC-SHA384
      * ECDHE-PSK-CHACHA20-POLY1305

      Supported ecdsa_ciphers_cc310_legacy suites (AES256 and GCM removed):

      * ECDHE-ECDSA-AES128-SHA
      * ECDHE-ECDSA-AES128-SHA256
      * ECDHE-ECDSA-AES128-CCM8
      * ECDHE-ECDSA-AES128-CCM
      * ECDHE-ECDSA-CHACHA20-POLY1305
      * ECDHE-PSK-AES128-CBC-SHA256
      * ECDHE-PSK-CHACHA20-POLY1305

   .. tab:: TLS v1.3

      Supported ecdsa_ciphers_tls_13 suites:

      * TLS_CHACHA20_POLY1305_SHA256
      * TLS_AES_256_GCM_SHA384
      * TLS_AES_128_GCM_SHA256
      * TLS_AES_128_CCM_SHA256
      * TLS_AES_128_CCM_8_SHA256

AES256 is supported on all compatible board targets with the CMSE enabled because it enables :ref:`both nrf_cc3xx and nrf_oberon as backends<nrf_security_backends_oberon>` for devices with Arm CryptoCell.

The following combinations are *not* supported:

* RSA is not supported in applications that use the PSA Crypto configuration (which includes the CMSE).
* AES CCM-256 and AES GCM are not supported for nRF52840 and for the nRF91 Series devices when using the :ref:`nrf_cc3xx legacy <nrf_security_backends_cc3xx>` crypto configuration.

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
   This is automatically enforced in the configuration files for board targets with CMSE enabled (``*/ns`` :ref:`variant <app_boards_names>`).

TAP adapter
===========

The sample uses the :ref:`lib_eth_rtt` library in the |NCS| to transfer Ethernet frames to the connected PC.
The PC must parse the incoming RTT data and dispatch these packets to a running TAP network device.
This functionality is called *TAP adapter*.

.. tabs::

   .. tab:: Windows

      There is currently no direct support for TAP adapters.
      However, you can still follow the steps given in the `Testing`_ section using a Linux distribution running inside a virtual machine.

   .. tab:: Linux

      The TAP adapter functionality is included in the `Ethernet over RTT for Linux`_ executable, named :file:`eth_rtt_link`, located in the :file:`samples/crypto/psa_tls` folder.
      You must pass the development kit's SEGGER ID and the TAP IPv4 as parameters when calling the executable.
      See the examples in the `Testing`_ section.

      When using an nRF5340 development kit, if :file:`eth_rtt_link` cannot start the RTT connection, pass the ``_SEGGER_RTT`` RAM block address as a parameter using ``--rttcbaddr``, as shown in the following example:

      .. code-block:: console

         sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.2 --rttcbaddr 0x20002000

      You can find the ``_SEGGER_RTT`` RAM address in the :file:`.map` file.


OpenSSL
=======

Use the OpenSSL command-line interface to perform the peer network operations when testing this sample.

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

   .. tab:: Server test

      1. Start a terminal emulator like the `Serial Terminal app`_ and connect to the used serial port with the standard UART settings.
         See :ref:`test_and_optimize` for more information.
      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as a superuser with your development kit's SEGGER ID and the following IPv4 address as parameters:

         .. code-block:: console

           sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.2

      #. Use OpenSSL to perform the `client` connection and handshake operation.

         .. tabs::

            .. tab:: TLS v1.2

               .. code-block:: console

                  openssl s_client -connect 192.0.2.1:4243 -cipher ECDHE-ECDSA-AES128-SHA256 -CAfile certs/ecdsa/root_cert.pem

               .. note::

                  If the sample has been built with an RSA certificate, use this OpenSSL command:

                  .. code-block:: console

                     openssl s_client -connect 192.0.2.1:4243 -cipher AES128-SHA256 -CAfile certs/rsa/root_cert.pem

            .. tab:: TLS v1.3

               .. code-block:: console

                  openssl s_client -connect 192.0.2.1:4243 -tls1_3 -verifyCAfile certs/ecdsa/root_cert.pem -ciphersuites TLS_AES_128_GCM_SHA256

         For visualizing a list of the available :ref:`crypto_tls_suites` for OpenSSL, use the following command:

         .. code-block:: console

            openssl ciphers

      #. Type ``Nordic Semiconductor`` into the OpenSSL connection session to send ``Nordic Semiconductor`` as an encrypted message to the server.
      #. Check that the TLS sample returns ``Nordic Semiconductor`` in the OpenSSL session.
      #. Check in the terminal emulator that 21 bytes were successfully received and returned.


   .. tab:: Client test

      1. Start a terminal emulator like the `Serial Terminal app`_ and connect to the used serial port with the standard UART settings.
         See :ref:`test_and_optimize` for more information.
      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as a superuser with your development kit's SEGGER ID and the following IPv4 address as parameters:

         .. code-block:: console

           sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.1

      #. Use OpenSSL to start the server, which waits for the `client` connection and handshake operation.

         .. tabs::

            .. tab:: TLS v1.2

               .. code-block:: console

                  openssl s_server -accept 4243 -cipher ECDHE-ECDSA-AES128-SHA256 -cert certs/ecdsa/cert.pem -key certs/ecdsa/cert.key

               .. note::

                  If the sample has been built with an RSA certificate, use this OpenSSL command:

                  .. code-block:: console

                     openssl s_server -accept 4243 -cipher AES128-SHA256 -cert certs/rsa/cert.pem -key certs/rsa/cert.key

            .. tab:: TLS v1.3

               .. code-block:: console

                  openssl s_server -accept 4243 -tls1_3 -cert certs/ecdsa/cert.pem -key certs/ecdsa/cert.key -ciphersuites TLS_AES_128_GCM_SHA256

         For visualizing a list of the available cipher suites for openssl, use the following command:

         .. code-block:: console

           openssl ciphers

      #. Type ``Nordic Semiconductor`` into the OpenSSL connection session to send ``Nordic Semiconductor`` as an encrypted message to the client.
      #. Check that the TLS sample returns ``Nordic Semiconductor`` in the OpenSSL session.
      #. Check in the terminal emulator that 21 bytes were successfully received and returned.


   .. tab:: DTLS server test

      Use ``dtls.conf`` overlay when building the sample to enable DTLS support.

      1. Start a terminal emulator like the `Serial Terminal app`_ and connect to the used serial port with the standard UART settings.
         See :ref:`test_and_optimize` for more information.
      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as a superuser with your development kit's SEGGER ID and the following IPv4 address as parameters:

         .. code-block:: console

           sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.2

      #. Use OpenSSL to perform the `client` connection and handshake operation.

         .. code-block:: console

           openssl s_client -dtls -connect 192.0.2.1:4243 -cipher ECDHE-ECDSA-AES128-SHA256 -CAfile certs/ecdsa/root_cert.pem

         .. note::

            If the sample has been built with an RSA certificate, use this OpenSSL command:

            .. code-block:: console

               openssl s_client -dtls -connect 192.0.2.1:4243 -cipher AES128-SHA256 -CAfile certs/rsa/root_cert.pem

         For visualizing a list of the available cipher suites for openssl, use the following command:

         .. code-block:: console

           openssl ciphers

      #. Type ``Nordic Semiconductor`` into the OpenSSL connection session to send ``Nordic Semiconductor`` as an encrypted message to the server.
      #. Check that the TLS sample returns ``Nordic Semiconductor`` in the OpenSSL session.
      #. Check in the terminal emulator that 21 bytes were successfully received and returned.


   .. tab:: DTLS client test

      Use ``dtls.conf`` overlay when building the sample to enable DTLS support.

      1. Start a terminal emulator like the `Serial Terminal app`_ and connect to the used serial port with the standard UART settings.
         See :ref:`test_and_optimize` for more information.
      #. Observe the logs from the application using the terminal emulator.
      #. Start the ``eth_rtt_link`` executable as a superuser with your development kit's SEGGER ID and the following IPv4 address as parameters:

         .. code-block:: console

           sudo ./eth_rtt_link --snr 960010000 --ipv4 192.0.2.1

      #. Use OpenSSL to start the server, which waits for the `client` connection and handshake operation.

         .. code-block:: console

           openssl s_server -dtls -accept 4243 -cipher ECDHE-ECDSA-AES128-SHA256 -cert certs/ecdsa/cert.pem -key certs/ecdsa/cert.key

         .. note::

            If the sample has been built with an RSA certificate, use this OpenSSL command:

            .. code-block:: console

               openssl s_server -dtls -accept 4243 -cipher AES128-SHA256 -cert certs/rsa/cert.pem -key certs/rsa/cert.key

         For visualizing a list of the available cipher suites for openssl, use the following command:

         .. code-block:: console

           openssl ciphers

      #. Type ``Nordic Semiconductor`` into the OpenSSL connection session to send ``Nordic Semiconductor`` as an encrypted message to the client.
      #. Check that the TLS sample returns ``Nordic Semiconductor`` in the OpenSSL session.
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
