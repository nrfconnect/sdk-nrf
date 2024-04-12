.. _lib_hw_unique_key:

Hardware unique key
###################

.. contents::
   :local:
   :depth: 2

The hardware unique key library manages symmetric root keys on devices with the Arm CryptoCell security subsystem.

Functionality
*************

Hardware unique keys (HUKs) are device-specific keys that you can use with functions for key derivation to generate other keys.
Applications can use the generated keys for symmetric cryptographic operations.
By using HUKs, you can let your application use multiple keys without having to store them on the device, as they can be derived from a HUK, using a static label, when needed.

.. caution::
   Use hardware unique keys only for key derivation, never directly for symmetric cryptographic operations.

The library supports 3 different types of HUKs, depending on the device:

* Device root key (KDR).
  It is used for deriving general-purpose keys.
  In devices with the Arm CryptoCell CC310, it must be written to a specific KDR register during the boot process, using the :c:func:`hw_unique_key_load_kdr` function.
  The nRF Secure Immutable Bootloader does this automatically if you enable :kconfig:option:`CONFIG_HW_UNIQUE_KEY` in the bootloader image.
* Master key encryption key (MKEK).
  It is used for deriving Key Encryption Keys (KEKs), which are used to encrypt other keys when these are stored.
  It is provided to CryptoCell when it is used.
* Master key for encrypting external storage (MEXT).
  It is used to derive keys for encrypting data in external non-secure storage.
  It is provided to CryptoCell when it is used.

See the following table for an overview of the key types supported by each device:

.. list-table::
    :header-rows: 1

    * - Device
      - CryptoCell version
      - Key Management Unit
      - Supported HUK types
    * - nRF91 Series
      - CC310
      - Yes
      - KDR, MKEK, MEXT
    * - nRF5340
      - CC312
      - Yes
      - MKEK, MEXT
    * - nRF52840
      - CC310
      - No
      - KDR only

In devices with a Key Management Unit (KMU), like nRF91 Series or nRF5340, the keys reside in reserved slots in the KMU itself.
The KMU can make the keys non-readable and non-writable from the application, while still accessible by the Arm CryptoCell.

In devices without a KMU, like nRF52840, the bootloader writes the key to the Arm CryptoCell and locks the flash memory page where the key is stored.
In this case, only one key is supported.

Prerequisites
*************

To use hardware unique keys, you must first write them to the KMU or program them in the device firmware if no KMU is present.
You can also write the HUKs by programming the device with a debugger.
See :file:`tests/lib/hw_unique_key_tfm/write_kmu.py` for an example of programming the KMU with a debugger.

Usage
*****

The library provides a function for writing arbitrary keys and a function for writing random keys.

To use the library, enable the :kconfig:option:`CONFIG_HW_UNIQUE_KEY` option for the nRF Secure Immutable Bootloader image.
Additionally, you can enable the :kconfig:option:`CONFIG_HW_UNIQUE_KEY_RANDOM` option to enable the :c:func:`hw_unique_key_write_random` function and its dependencies, to generate random keys.

See :ref:`configure_application` for information on how to enable the required configuration options.

You can then use the HUKs through the APIs in the :ref:`CC3xx platform libraries<nrf_cc3xx_platform_readme>`.
You can also derive a key using :c:func:`hw_unique_key_derive_key`.

.. caution::
   It is strongly recommended to generate random keys on-chip to avoid any outside knowledge of the keys.
   If the application needs a specific key that the manufacturer also knows, and this key is also unique to the device, do not provide such a key as a HUK.
   Instead, encrypt this key with another key derived from a hardware unique key, or store it in its own KMU slot.

API documentation
*****************

| Header file: :file:`include/hw_unique_key.h`
| Source files: :file:`modules/lib/hw_unique_key/`

.. doxygengroup:: hw_unique_key
   :project: nrf
   :members:
