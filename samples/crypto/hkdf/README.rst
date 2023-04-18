.. _crypto_hkdf:

Crypto: HKDF
############

.. contents::
   :local:
   :depth: 2

The HMAC Key Derivation Function (HKDF) sample shows how to derive keys with the HKDF algorithm, using a sample key, salt, and additional data.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample performs the following operations:

1. Initialization of the Platform Security Architecture (PSA) API.

#. Key derivation:

   a. The input key is imported into the PSA crypto keystore.
   #. The output key is derived.

#. Cleanup:

   a. The input and the output keys are removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/hkdf`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
