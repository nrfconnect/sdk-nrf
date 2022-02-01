.. _crypto_ecdsa:

Crypto: ECDSA
#############

.. contents::
   :local:
   :depth: 2

The ECDSA sample shows how to sign and verify messages using SHA-256 as the hashing algorithm and the secp256r1 curve.

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
   #. A random Elliptic Curve Cryptography (ECC) key pair is generated and imported into the PSA crypto keystore.
   #. The public key of the ECDSA key pair is imported into the PSA crypto keystore.

#. ECDSA signing and verification:

   a. Signing is performed using the private key of the ECC key pair.
   #. The signature is verified using the exported public key.

#. Cleanup:

   a. The key pair and public key are removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdsa`

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
