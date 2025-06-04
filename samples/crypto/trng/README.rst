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

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample performs the following operations:

1. Initialization:

   a. The Platform Security Architecture (PSA) API is initialized.
   #. A random Elliptic Curve Cryptography (ECC) key pair is generated in the PSA crypto keystore for signing purposes.
   #. The public key of the ECC key pair is exported to the application.
   #. The exported public key is imported into the PSA crypto key storage to be used for verification.

#. ECDSA signing and verification:

   a. Signing is performed using the private key of the ECC key pair.
   #. The signature is verified using the exported public key.

#. Cleanup:

   a. The key pair and public key are removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdsa`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
