.. _crypto_aes:

Crypto: AES
###########

.. contents::
   :local:
   :depth: 2

Overview
********

The AES samples show how to perform AES encryption and decryption operations, as well as MAC calculation with different AES modes using the `AES - Advanced Encryption Standard`_ library.
AES library comes with several supported cryptography modes.
Therefore, the sample is divided into four parts.

Each of these parts presents the usage of a different mode:

* AES API for CTR
* AES API for CBC-MAC
* AEAD API for CCM
* AES API and AEAD API for all supported AES modes.

.. note::
   These examples can run in software or hardware, depending on the supported features of your SoC.
   They use the default backend for the specific platform, but you can test them using different backends.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes`

.. include:: /includes/build_and_run.txt

.. _crypto_aes_testing:

Testing different modes
=======================

The following process is applicable for testing AES CTR, AES CBC_MAC and AES CCM modes:

1. Skip this step if you are using an RTT Viewer. By default, the crypto examples are configured to use RTT for logging.
   Start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

   * Baud rate: 115.200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. _crypto_aes_testing_all:

Testing all AES modes
=====================

To test All AES modes using command-line interface, perform the following steps:

1. Compile and program the application.
#. Start a terminal emulator like PuTTY (recommended) with the following Terminal settings, and establish the connection:

   * Serial: for UART or USB CDC ACM transport.
   * Telnet, port 19021: for RTT transport. For establishing an RTT session, use ``JLink.exe`` and not J-Link Viewer.
#. On the command-line interface, press the Tab button to see all available commands.
#. To test the functionality of each AES mode, run the following commands:

   * For CBC, CFB, CTR and ECB modes:

	 .. code-block:: console

	    example aes <AES mode>

   * For CCM, CCM*, EAX and GCM modes:

	 .. code-block:: console

	    example aead <AEAD mode>

   * For CBC-MAC and CMAC modes:

	 .. code-block:: console

	    example mac <MAC mode>
#. Change the key size and test the above functionality again to compare results:

   .. code-block:: console

      example key_size <size>

   Available sizes are 128_bit, 192_bit, and 256_bit. Modes implemented by the CC310 backend support only the 128-bit key.
#. Use the Tab key to autocomplete and prompt commands.
#. If your chip features Cryptocell®, you can change between the backends (CC310 <-> mbed TLS) realizing a particular AES mode. Compile and test again to compare results.
