.. _crypto_aes_gcm:

Crypto: AES GCM
###############

.. contents::
   :local:
   :depth: 2

The AES GCM sample shows how to perform authenticated encryption and decryption operations using the GCM algorithm and a 128-bit key.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuapp_ns, nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160_ns, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. include:: /includes/tfm.txt

Overview
********

The sample performs the following operations:

1. Initialization:

   a. The Platform Security Architecture (PSA) API is initialized.
   #. A random AES key is generated and imported into the PSA crypto keystore.

#. Encryption and decryption of a sample plaintext:

   a. A random initialization vector (IV) is generated.
   #. Authenticated encryption is performed.
   #. Authenticated decryption is performed.

#. Cleanup:

   a. The AES key is removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_gcm`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
