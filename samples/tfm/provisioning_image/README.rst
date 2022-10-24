.. _provisioning_image:

Provisioning image
##################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to initialize the provisioning process of a Trusted Firmware M(TF-M) image.
The provioning process sets the device in the Provisioning Platform Root of Trust(PRoT) Security Lifecycle as defined in the security model of Platform Security Architecture(PSA).
This sample does not include a TF-M image, it is a Zephyr image which is required to run before the TF-M image in order to allow the provisioning of the TF-M image later.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The :ref:`lib_hw_unique_key` and the :ref:`lib_identity_key` libraries are required to generate and store the Master Key Encryption Key (MKEK) and the identity key into KMU.

.. note::
   Once the required identity key is provisioned on the device, only the code pages should be erased as ERASEALL removes the identity key from the system.

Overview
********

The PSA security model defines the PRoT Security Lifecycle Provisioning as the state where the device platform security parameters are generated.
This sample performs the transition from the PRoT Security Lifecycle "Device assembly and test" to the PRoT Security Lifecycle "PRoT Provisioning".
This enables a TF-M image in a later stage to transition to the PRoT Security Lifecycle "Secured".

The sample performs the following operations:

1. The device is verified to be in PRoT Security Lifecycle "Device assembly and test".
#. The device is transitioned to the PRoT Security Lifecycle "PRoT Provisioning".
#. The random hardware unique keys(HUKs) are generated and stored in the Key Management Unit (KMU).
#. A random identity key of type secp256r1 is generated and stored in the KMU.
#. The identity key is verified to be stored in KMU.

The Hardware Unique Keys(HUKs) include the Master Key Encryption Key (MKEK) and the Master key for Encrypting EXternal storage (MEXT).

The identity key is stored in the KMU in encrypted form using the Hardware Unique Key (HUK) Master Key Encryption Key (MKEK).
The identity key is used as the Initial Attestation Key as described in the PSA security model.


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

             Successfully verified PSA lifecycle state ASSEMBLY!
             Successfully switched to PSA lifecycle state PROVISIONING!
             Generating random HUK keys (including MKEK)
             Writing the identity key to KMU
             Success!

         If an error occurs, the sample prints a message and raises a kernel panic.

Dependencies
************

The following libraries are used:

* :ref:`lib_hw_unique_key`
* :ref:`lib_identity_key`
