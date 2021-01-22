.. _crypto_ecdsa:

Crypto: ECDSA
#############

.. contents::
   :local:
   :depth: 2

Overview
********

The ECDSA sample shows how to sign/verify messages using SHA256 as the hashing algorithm and the secp256r1 curve.


First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.
   #. A random ECDSA keypair is generated an imported in the PSA crypto keystore
   #. The public key of the ECDSA keypair is imported in the PSA crypto keystore

Then, ECDSA signing/verification is performed:
   #. Signing is performed using the ECDSA keypair.
   #. Verification of the signagure is performed using the exported public key.

Afterwards, the sample performs cleanup:
   #. The ECDSA keypair and public key are removed from the PSA crypto keystore.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdsa`

.. include:: /includes/build_and_run.txt

Testing
=======

Follow these steps to test the ECDSA example:

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
