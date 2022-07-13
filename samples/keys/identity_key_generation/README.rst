.. _identity_key_generate:

Identity key generation
#######################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to generate a random device-specific identity using :ref:`lib_identity_key`, which is then stored in the Key Management Unit (KMU).

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The :ref:`lib_hw_unique_key` library is required to generate and store the prerequisite Master Key Encryption Key (MKEK) into KMU.

.. note::
   Once the required identity key is provisioned on the device, only the code pages should be erased as ERASEALL removes the identity key from the system.

Overview
********

The identity key is stored in the KMU in encrypted form using the Hardware Unique Key (HUK) Master Key Encryption Key (MKEK).
The sample also demonstrates how to generate a random MKEK and store it in KMU.

The sample performs the following operations:

1. The random hardware unique keys(HUKs) are generated and stored in the KMU.
#. A random identity key of type secp256r1 is generated and stored in the KMU.
#. The identity key is verified to be stored in KMU.

Configuration
*************

|config|


Building and running
********************

.. |sample path| replace:: :file:`samples/keys/identity_key_generate`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Observe the following output:

         .. code-block:: console

             Generating random HUK keys
             Writing the identity key to KMU
             Success!

         If an error occurs, the sample prints a message and raises a kernel panic.

Dependencies
************

The following libraries are used:

* :ref:`lib_hw_unique_key`
* :ref:`lib_identity_key`
