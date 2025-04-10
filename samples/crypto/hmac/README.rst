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

.. table-from-sample-yaml::

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

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
