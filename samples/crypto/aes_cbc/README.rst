.. _crypto_aes_cbc:

Crypto: AES CBC
###############

.. contents::
   :local:
   :depth: 2

The AES CBC sample shows how to perform AES encryption and decryption operations using the CBC block cipher mode without padding and a 128-bit AES key.

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
   #. A random AES key is generated and imported into the PSA crypto keystore.

#. Encryption and decryption of a sample plaintext:

   a. A random initialization vector (IV) is generated.
   #. Encryption is performed.
   #. Decryption is performed.

#. Cleanup:

   a. The AES key is removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_cbc`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
