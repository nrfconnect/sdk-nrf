.. _lib_identity_key:

Identity key
############

.. contents::
   :local:
   :depth: 2

The identity key library manages an asymmetric key used for identity services on devices with the Arm CryptoCell and KMU peripherals.
It's used to provision identity keys and can only be used by a Zephyr image in Secure Processing Environment (SPE).
It is not supported from images for Non-Secure Processing Environment (NSPE), from a Trusted Firmware-M image, or from MCUboot.

The identity key is equivalent to the Initial Attestation Key (IAK), as described in the ARM Platform Security Model 1.1, when Trusted Firmware-M (TF-M) is enabled.
TF-M has access to the identity key using internal APIs and does not need to use this library.

Functionality
*************

This library manages identity keys, which are asymmetric keys intended to provide a unique identity to a device.
The identity key is designed to be unique and is provisioned either device-generated or otherwise in a secure manner during production.
Two reserved slots of the Key Management Unit (KMU) peripheral are used to store the identity key in order to protect its integrity.
The identity key is stored in an encrypted form using a Key Encryption Key (KEK) derived by the Hardware Unique key (HUK) Master Key Encryption Key (MKEK).

.. caution::
   The identity key must not be shared. Leaking this leaks the identity of the device.

.. caution::
   The identity key is stored in the KMU and will therefore be erased by ERASEALL.

Prerequisites
*************

To use the identity key APIs, you must first generate or provision HUK keys on the device.


API documentation
*****************

| Header file: :file:`include/identity_key.h`
| Source files: :file:`modules/lib/identity_key/`

.. doxygengroup:: identity_key
   :project: nrf
   :members:
