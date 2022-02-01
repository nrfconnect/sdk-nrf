.. _crypto_hmac:

Crypto: HMAC
############

.. contents::
   :local:
   :depth: 2

The HMAC sample shows how to generate a message authentication code using the HMAC algorithm and then verify it.
The HMAC algorithm in this sample uses SHA-256.

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

1. Initialization of the Platform Security Architecture (PSA) API.

#. Signing and verification of a sample plaintext:

   a. A secret key is generated and imported into the PSA crypto keystore.
   #. HMAC signing is performed.
   #. HMAC verification is performed.

#. Cleanup:

   a. The secret key is removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/hmac`

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
