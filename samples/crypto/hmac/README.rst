.. _crypto_hmac:

Crypto: HMAC
############

.. contents::
   :local:
   :depth: 2

The HMAC sample shows how to perform HMAC signing and verification operations using the SHA-256 hashing algorithm.

Overview
********

The sample follows these steps:

1. First, the sample initializes the Platform Security Architecture (PSA) API.

#. Then, it signs and verifies a sample plaintext:

   a. A random HMAC key is generated and imported into the PSA crypto keystore.
   #. HMAC signing is performed.
   #. HMAC verification is performed.

#. Afterwards, it performs cleanup:

   a. The HMAC key is removed from the PSA crypto keystore.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns, nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160ns, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. include:: /includes/tfm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/hmac`

.. include:: /includes/build_and_run_tfm.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
