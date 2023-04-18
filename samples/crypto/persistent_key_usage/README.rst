.. _crypto_persistent_key:

Crypto: Persistent key storage
##############################

.. contents::
   :local:
   :depth: 2

The persistent key sample shows how to generate a persistent key using the Platform Security Architecture (PSA) APIs.
Persistent keys are stored in the Internal Trusted Storage (ITS) of the device and retain their value between resets.
A persistent key becomes unusable when the ``psa_destroy_key`` function is called.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

In this sample, an AES 128-bit key is created.
Persistent keys can be of any type supported by the PSA APIs.

The sample performs the following operations:

1. Initialization of the Platform Security Architecture (PSA) API.

#. Generation of a persistent AES 128-bit key.

#. Cleanup.
   The AES key is removed from the PSA crypto keystore.

.. note::
   The read-only type of persistent keys cannot be destroyed with the ``psa_destroy_key`` function.
   The ``PSA_KEY_PERSISTENCE_READ_ONLY`` macro is used for read-only keys.
   The key ID of a read-only key is writable again after a full erase of the device memory.
   Use the ``west -v flash --erase`` command for the full erase.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/persistent_key_usage`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
