.. _crypto_persistent_key:

Crypto: Persistent key usage
############################

.. contents::
   :local:
   :depth: 2

The persistent key sample shows how to generate a persistent key using the Platform Security Architecture (PSA) APIs.
Persistent keys are stored in the Internal Trusted Storage (ITS) of the device and retain their value between resets.
The implementation of the PSA ITS API is provided in one of the following ways, depending on your configuration:

* Through TF-M using Internal Trusted Storage and Protected Storage services.
* When building without TF-M: using either Zephyr's :ref:`secure_storage` subsystem or the :ref:`trusted_storage_readme` library.

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

#. Removal of the key from RAM.

#. Encryption and decryption of a message using the key.

#. Cleanup.
   The AES key is removed from the PSA crypto keystore.

.. note::
   The read-only type of persistent keys cannot be destroyed with the ``psa_destroy_key`` function.
   The ``PSA_KEY_PERSISTENCE_READ_ONLY`` macro is used for read-only keys.
   The key ID of a read-only key is writable again after a full erase of the device memory.
   Use the ``west -v flash --erase`` command for the full erase.

.. note::
   Builds without TF-M and all nRF54L15 builds use the :ref:`hardware unique key (HUK) <lib_hw_unique_key>` to encrypt the key before storing it.

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

Dependencies
************

* PSA APIs:

   * :file:`psa/crypto.h`

* Builds without TF-M use the :ref:`secure_storage` subsystem as the PSA Secure Storage API
  provider.

   * The :ref:`lib_hw_unique_key` is used to encrypt the key before storing it.
