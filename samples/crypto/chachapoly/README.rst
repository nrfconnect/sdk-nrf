.. _crypto_chacha_poly:

Crypto: Chacha20-Poly1305 example
#################################

.. contents::
   :local:
   :depth: 2

Overview
********

The Chacha20-Poly1305 sample shows how to perform authenticated encryption and decryption operations. The sample uses
additional data and a random nonce.

First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.
   #. A random Chacha20 key is generated and imported in the PSA crypto keystore.

Then, the sample encrypts/decrypts a sample plaintext:
   #. A random nonce is generated.
   #. Authenticated encryption is performed.
   #. Authenticated decryption is performed.

Afterwards, the sample performs cleanup:
   #. The AES key is removed from the PSA crypto keystore

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/chacha_poly`

.. include:: /includes/build_and_run.txt

Testing
=======

Follow these steps to test the Chacha20-Poly1305 example:

1. Start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

   * Baud rate: 115.200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
