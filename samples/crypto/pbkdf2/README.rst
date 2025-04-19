.. _crypto_pbkdf2:

Crypto: PBKDF2
##############

.. contents::
   :local:
   :depth: 2

The Password Based Key Derivation Function (PBKDF2) sample shows how to derive keys with the PBKDF2 algorithm, using a sample password salt, and iteration count.
The underlying pseudorandom function (PRF) used in this sample is HMAC with SHA-256.

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

   a. Imports the input password into the PSA crypto keystore.
   #. Derives the output key.

#. Cleanup:

   a. The input password is removed from the PSA crypto keystore.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/pbkdf2`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
