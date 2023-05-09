.. _provisioning_image:

Provisioning image
##################

.. contents::
   :local:
   :depth: 2

Running the provisioning image sample will initialize the provisioning process of a device in a manner compatible with Trusted Firmware-M (TF-M).
This sample does not include a TF-M image, it is a Zephyr image intended to be flashed, run, and erased before the TF-M image is flashed.

After completion, the device is in the Platform Root-of-Trust (PRoT) security lifecycle state called **PRoT Provisioning**.
For more information about the PRoT security lifecycle, see Arm's Platform Security Model 1.1 defined in the Platform Security Architecture (PSA).

When built for the nrf5340dk_nrf5340_cpuapp target, this image by default also includes the :ref:`provisioning image for network core<provisioning_image_net_core>` sample as a child image for the network core (nrf5340dk_nrf5340_cpunet target).
The child image demonstrates how to disable the debugging access on the network core by writing to the UICR.APPROTECT register.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The sample also requires the following libraries to generate and store the master key encryption key (MKEK) and the identity key into the key management unit (KMU):

 * :ref:`lib_hw_unique_key`
 * :ref:`lib_identity_key`

.. note::
   Once the required identity key is provisioned on the device, the UICR should not be erased by ERASEALL for example, as that removes the identity key from the system.

Overview
********

The PSA security model defines the PRoT security lifecycle states.
This sample performs the transition from the PRoT security lifecycle state **Device Assembly and Test** to the **PRoT Provisioning** state.

PRoT Provisioning is a state where the device platform security parameters are generated.
This enables a TF-M image to transition to the PRoT security lifecycle state **Secured** at a later stage.

The sample performs the following operations:

1. The device is verified to be in the Device Assembly and Test state.
#. The device is transitioned to the PRoT Provisioning state.
#. Random hardware unique keys (HUKs) are generated and stored in the key management unit (KMU).
#. A random identity key of type secp256r1 is generated and stored in the KMU.
#. The identity key is verified to be stored in the KMU.

The hardware unique keys (HUKs) include the master key encryption key (MKEK) and the master key for encrypting external storage (MEXT).

The identity key is stored in the KMU in encrypted form using the master key encryption key (MKEK).
The identity key is used as the Initial Attestation Key (IAK) as described in the PSA security model.


Configuration
*************

|config|


Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/provisioning_image`

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
