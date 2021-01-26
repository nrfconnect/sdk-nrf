.. _crypto_rsa:

Crypto: RSA
###########

.. contents::
   :local:
   :depth: 2

Overview
********

The RSA sample shows how to perform singing and verification of a sample plaintext using a 1024-bit key.

First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.
   #. A random RSA keypair is generated an imported in the PSA crypto keystore
   #. The public key of the RSA keypair is imported in the PSA crypto keystore

Then, RSA signing/verification is performed:
   #. Signing is performed using the RSA keypair.
   #. Verification of the signagure is performed using the exported public key.

Afterwards, the sample performs cleanup:
   #. The RSA keypair and public key are removed from the PSA crypto keystore.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/rsa`

.. include:: /includes/build_and_run.txt

Testing
=======

Follow these steps to test the RSA example:

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
   If you are using RTT, skip the first step of the testing procedure
