.. _crypto_psa_its:

Crypto: PSA ITS
##############################

.. contents::
   :local:
   :depth: 2

This sample shows how to use the Platform Security Architecture (PSA) Internal Trusted Storage (ITS) APIs.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

Demonstration of how to use the PSA ITS APIs to store and retrieve data.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/psa_its`

.. include:: /includes/build_and_run_ns.txt

.. note::
   Use the ``west -v flash --erase`` command for the full erase.

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
   * :file:`psa/internal_trusted_storage.h`

* Builds without TF-M use the :ref:`trusted_storage_readme` library

   * The :ref:`lib_hw_unique_key` may be used to encrypt the key before storing it.
