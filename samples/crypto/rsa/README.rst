.. _crypto_rsa:

Crypto: RSA
###########

.. contents::
   :local:
   :depth: 2

The RSA sample shows how to perform signing and verification of a sample plaintext using a 4096-bit key.

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
   #. An RSA key pair is imported into the PSA crypto keystore.
   #. The public key is imported into the PSA crypto keystore.

      .. note::
         The keys used in the sample are for demonstration purposes only and must never be used in a product.

#. RSA signing and verification:

   a. Signing is performed using the RSA private key.
   #. The signature is verified using the public key.

#. Cleanup:

   a. The RSA key pair and public key are removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/rsa`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
