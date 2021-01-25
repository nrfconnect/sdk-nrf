.. _crypto_ecdh:

Crypto: ECDH
############

.. contents::
   :local:
   :depth: 2

Overview
********

The ECDH sample shows how to perform an Elliptic-curve Diffie–Hellman key exhange to allow two parties to obtain a common secret.

The sample peforms an ECDH exchange:
   #. The Platform Security Architecture (PSA) API is initialized.
   #. Two ECDH keypairs are generated and imported to the PSA keystore.
   #. ECDH key echange is performed.
   #. The keypairs are removed from the PSA keystore.

First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.
   #. Two random ECDH keypairs are generated and imported in the PSA crypto keystore

First, the sample signs a sample plaintext:
   #. Signing is performed.
   #. The keypair is removed from the PSA keystore.

Then, ECDH key exchange is performed:
   #. ECDH key echange is performed using the generated keypairs.

Afterwards, the sample performs cleanup:
   #. The ECDH keypairs are removed from the PSA crypto keystore.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdh`

.. include:: /includes/build_and_run.txt

Testing
=======================

Follow these steps to test the ECDH example:

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
