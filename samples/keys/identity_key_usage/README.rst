.. _identity_key_usage:

Identity key usage
##################

.. contents::
   :local:
   :depth: 2

This sample shows how to use a previously written identity key with the PSA crypto APIs.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The :ref:`lib_hw_unique_key` library is required to generate the prerequisite Master Key Encryption Key (MKEK).
The :ref:`lib_identity_key` library is required to provision the identity key in KMU.
Both these operations can be done by running the :ref:`identity_key_generate` sample.

.. note::
   Once the required identity key is provisioned on the device, only the code pages should be erased as ERASEALL removes the identity key from the system.

Overview
********

The identity key is an asymmetric key of type secp256r1 which can be used for attestation
services.
This sample shows how to load the identity key into the PSA crypto keystore for usage.

The sample performs the following operations:

1. The Platform Security Architecture (PSA) API is initialized.
#. The identity key is verified to be in the KMU.
#. The identity key is read from the KMU.
#. The identity key is imported to the PSA crypto keystore.
#. The public key is exported from the identity key.


Configuration
*************

|config|

Building and running
********************

.. |sample path| replace:: :file:`samples/keys/identity_key_usage`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. Reset the kit.
#. Observe the following output:

   .. code-block:: console

      Initializing PSA crypto.
      Reading the identity key.
      Importing the identity key into PSA crypto.
      Exporting the public key corresponding to the identity key.
      Success!

   If an error occurs, the sample prints a message and raises a kernel panic.

Dependencies
************

This sample uses the following libraries:

* :ref:`lib_identity_key`
* :ref:`nrf_security`
