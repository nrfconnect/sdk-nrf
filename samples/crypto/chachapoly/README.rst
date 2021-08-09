.. _crypto_chacha_poly:

Crypto: Chacha20-Poly1305 example
#################################

.. contents::
   :local:
   :depth: 2

The Chacha20-Poly1305 sample shows how to perform authenticated encryption and decryption operations.
The sample uses additional data and a random nonce.

Overview
********

The sample follows these steps:

1. First, the sample performs initialization:

   a. The Platform Security Architecture (PSA) API is initialized.
   #. A random Chacha20 key is generated and imported into the PSA crypto keystore.

#. Then, it encrypts and decrypts a sample plaintext:

   a. A random nonce is generated.
   #. Authenticated encryption is performed.
   #. Authenticated decryption is performed.

#. Afterwards, it performs cleanup:

   a. The AES key is removed from the PSA crypto keystore.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuapp_ns, nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160ns, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. include:: /includes/tfm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/chacha_poly`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
