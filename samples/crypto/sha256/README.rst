.. _crypto_sha256:

Crypto: SHA-256
###############

.. contents::
   :local:
   :depth: 2

The SHA-256 sample shows how to calculate and verify hashes.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample performs the following operations:

1. Initialization of the Platform Security Architecture (PSA) API.

#. Calculation and verification of a SHA-256 hash on a sample plaintext:

   a. The SHA-256 hash is calculated.
   #. The SHA-256 hash is verified.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/sha256`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
