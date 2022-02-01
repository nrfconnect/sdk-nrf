.. _crypto_rsa:

Crypto: RSA
###########

.. contents::
   :local:
   :depth: 2

The RSA sample shows how to perform singing and verification of a sample plaintext using a 1024-bit key.

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
   #. A random RSA key pair is generated and imported into the PSA crypto keystore.
   #. The public key of the RSA key pair is imported into the PSA crypto keystore.

#. RSA signing and verification:

   a. Signing is performed using the RSA private key.
   #. The signature is verified using the exported public key.

#. Cleanup:

   a. The RSA key pair and public key are removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/rsa`

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
