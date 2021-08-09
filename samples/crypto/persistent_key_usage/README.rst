.. _crypto_persistent_key:

Crypto: Persistent key storage
##############################

.. contents::
   :local:
   :depth: 2

The persistent key sample shows how to generate a persistent key using the Platform Security Architecture (PSA) APIs.
Persistent keys are stored in the Internal Trusted Storage (ITS) of the device and retain their value between resets.
A persistent key becomes unusable when the ``psa_destroy_key`` function is called.

Overview
********

In this sample, an AES 128-bit key is created.
Persistent keys can be of any type supported by the PSA APIs.

1. First, the sample initializes the Platform Security Architecture (PSA) API.

#. Then, it generates a persistent AES 128-bit key.

#. Afterwards, the sample performs cleanup.
   The AES key is removed from the PSA crypto keystore.


.. note::
   There is a read-only type of persistent keys which cannot be destroyed with ``psa_destroy_key``.
   The ``PSA_KEY_PERSISTENCE_READ_ONLY`` persistence macro is used for read-only keys.
   The key ID of a read-only key is writable again after a full erase of the device memory is performed using the ``west -v flash --erase`` command.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuapp_ns, nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160ns, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. include:: /includes/tfm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/persistent_key_usage`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
