.. _crypto_ecdh:

Crypto: ECDH
############

.. contents::
   :local:
   :depth: 2

The ECDH sample shows how to perform an Elliptic-curve Diffie–Hellman key exchange to allow two parties to obtain a shared secret.

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
   #. Two random Elliptic Curve Cryptography (ECC) key pairs are generated and imported into the PSA crypto keystore.

#. ECDH key exchange using the generated key pairs.

#. Cleanup:

   a. The key pairs are removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdh`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
