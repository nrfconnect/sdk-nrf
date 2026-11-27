.. _lib_hw_unique_key:

Hardware unique key
###################

.. contents::
   :local:
   :depth: 2

The hardware unique key library manages symmetric root keys on devices with the Arm CryptoCell or CRACEN security subsystem.

Overview
********

Hardware unique keys (HUKs) are device-specific keys that you use for key derivation to generate other keys.
Applications can use the derived keys for symmetric cryptographic operations.
By using HUKs, your application can use multiple keys without storing each one on the device, because they can be derived from a HUK with a static label when needed.

.. caution::
   Use hardware unique keys only for key derivation, never directly for symmetric cryptographic operations.

For a higher-level description of HUK-based key storage, see :ref:`key_storage_huk`.

Supported HUK types
===================

The |NCS| supports the following types of HUKs through this library:

.. list-table:: Hardware Unique Key (HUK) types
   :widths: auto
   :header-rows: 1

   * - HUK type
     - Purpose
     - Storage location
     - Additional notes
   * - Master Key Encryption Key (MKEK)
     - Deriving Key Encryption Keys (KEKs) for encrypting :ref:`Internal Trusted Storage (ITS) <ug_psa_certified_api_overview_secstorage>` entries
     - | - :ref:`key_storage_kmu` (CryptoCell devices)
       | - Derived by CRACEN from the IKG seed in KMU (nRF54L Series)
       | - Provided to CryptoCell or CRACEN when it is used
     - Recommended for nRF91 Series devices, nRF5340, and nRF54L Series devices
   * - Master Key for External Storage (MEXT)
     - Deriving keys for encrypting data in external non-secure storage (flash) or :ref:`Protected Storage <ug_psa_certified_api_overview_secstorage>`
     - | - :ref:`key_storage_kmu` (CryptoCell devices)
       | - Derived by CRACEN from the IKG seed in KMU (nRF54L Series)
       | - Provided to CryptoCell or CRACEN when it is used
     - Recommended for nRF91 Series devices, nRF5340, and nRF54L Series devices
   * - CRACEN Isolated Key Generator (IKG)
     - Deriving special hardware keys for CRACEN operations
     - :ref:`key_storage_kmu` (IKG seed on nRF54L Series)
     - | - CRACEN derives MKEK and MEXT from the seed at every boot or power cycle
       | - Supported on nRF54L Series devices; see :ref:`ug_kmu_guides_cracen_ikg`
       | - Provision the seed with :c:func:`hw_unique_key_write` or :c:func:`hw_unique_key_write_random`
   * - Device Root Key (KDR)
     - Deriving general-purpose keys
     - Non-volatile memory locked by `ACL <nRF52840 DK Access Control Lists_>`_
     - | - Load into CryptoCell at boot with :c:func:`hw_unique_key_load_kdr` when :kconfig:option:`CONFIG_HW_UNIQUE_KEY_LOAD` is enabled in the nRF Secure Immutable Bootloader image.
       | - Supported on devices without a KMU (for example nRF52840)

Device support
==============

The following table summarizes which HUK types apply to each device family.
The library API exposes MKEK and MEXT on devices with a KMU, and KDR on devices without a KMU.

.. list-table::
    :header-rows: 1

    * - Device
      - Crypto driver
      - Key Management Unit
      - Supported HUK types
    * - nRF91 Series
      - CryptoCell CC310
      - Yes
      - MKEK, MEXT
    * - nRF54L Series
      - CRACEN
      - Yes
      - MKEK, MEXT, IKG
    * - nRF5340
      - CryptoCell CC312
      - Yes
      - MKEK, MEXT
    * - nRF52840
      - CryptoCell CC310
      - No
      - KDR only

On nRF54L Series devices, the library stores a single seed in the KMU.
CRACEN generates MKEK and MEXT from this seed at every boot.

On other devices with a KMU, the keys either reside in reserved KMU slots or are derived at runtime.
The KMU can make the keys non-readable and non-writable from the application, while still accessible by CryptoCell or CRACEN.

On devices without a KMU, the bootloader loads the KDR into CryptoCell and locks the flash page that stores the key material.
Only one root key is supported on such devices.

Prerequisites
*************

To use hardware unique keys, you must first write them to the KMU or program them in the device firmware if no KMU is present.
You can also write the HUKs by programming the device with a debugger.
See :ncs-file:`/tests/lib/hw_unique_key_tfm/write_kmu.py` for an example of programming the KMU with a debugger.

Configuration
*************

Enable the :kconfig:option:`CONFIG_HW_UNIQUE_KEY` Kconfig option to build the library.

The following options are also available:

:kconfig:option:`CONFIG_HW_UNIQUE_KEY_RANDOM`
   Enables the :c:func:`hw_unique_key_write_random` function and its dependencies.

:kconfig:option:`CONFIG_HW_UNIQUE_KEY_LOAD`
   Enables loading of the KDR from a dedicated flash partition on boot.
   Enable this option together with :kconfig:option:`CONFIG_HW_UNIQUE_KEY` in the nRF Secure Immutable Bootloader image on devices without a KMU.

:kconfig:option:`CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT`
   Writes HUKs during PSA driver initialization when building without TF-M.

When building with TF-M, HUK provisioning and some derivations are handled inside the SPE; see :ref:`hw_unique_key_tfm_derivation`.

See :ref:`configure_application` for information on how to enable the required configuration options.

Usage
*****

After enabling the library, provision HUK material on the device, then derive application keys from the appropriate HUK slot.
On nRF54L Series devices, derive application keys from MKEK or MEXT unless component documentation specifies another slot.

Provisioning
============

The library provides a function for writing arbitrary keys and a function for writing random keys.

On CryptoCell devices with a KMU, write MKEK and MEXT with :c:func:`hw_unique_key_write` or :c:func:`hw_unique_key_write_random`.
On nRF54L Series devices, write the IKG seed with the same functions.
When TF-M is enabled on nRF54L Series devices, provisioning can be handled in the SPE; see the :ref:`TF-M Provisioning image <provisioning_image>`.

On devices without a KMU, enable :kconfig:option:`CONFIG_HW_UNIQUE_KEY_LOAD` together with :kconfig:option:`CONFIG_HW_UNIQUE_KEY` in the nRF Secure Immutable Bootloader image so that :c:func:`hw_unique_key_load_kdr` runs at boot.
After writing a key from the application, reboot the device so the bootloader can load the key into CryptoCell and lock the backing flash page.

.. caution::
   It is strongly recommended to generate random keys on-chip to avoid any outside knowledge of the keys.
   If the application needs a specific key that the manufacturer also knows, and this key is also unique to the device, do not provide such a key as a HUK.
   Instead, encrypt this key with another key derived from a hardware unique key, or store it in its own KMU slot.

Key derivation
==============

Call :c:func:`hw_unique_key_derive_key` to derive symmetric key material from a provisioned HUK.
The same call must use the same inputs to produce the same output, including:

* ``key_slot`` (the provisioned MKEK, MEXT, or KDR root)
* ``context`` and ``context_size`` (often ``NULL`` and ``0``)
* ``label`` and ``label_size`` (the exact byte sequence and length, including whether a terminating NUL is part of the label)
* ``output_size`` (the requested derived key length)

If any of these inputs change, including a firmware update that changes label handling, the derived key material changes as well.

Derivation uses the following backends, depending on the device:

* **CryptoCell (nRF91 Series and nRF5340, and KDR on devices without a KMU)** - :c:func:`nrf_cc3xx_platform_kmu_shadow_key_derive` from the :ref:`CC3xx platform libraries <nrf_cc3xx_platform_readme>`.
  On devices without a KMU, derive from the KDR loaded into CryptoCell.
* **CRACEN (nRF54L Series)** - SP800-108 counter mode with CMAC through the CRACEN PSA driver.

On devices with a KMU, applications typically derive from MKEK or MEXT.
On devices without a KMU, use KDR.

Labels used in the |NCS|
------------------------

Several components derive keys with fixed labels or inputs.
Use the same pattern if your application must derive compatible keys.

.. list-table::
   :widths: 22 12 30 24
   :header-rows: 1

   * - Use case
     - HUK slot
     - Label or input
     - Where
   * - TF-M built-in HUK (CryptoCell)
     - MEXT
     - ``TFM_HW_UNIQ_KEY`` with ``sizeof(label)`` (16 bytes, includes terminating NUL)
     - TF-M platform key loader in :ncs-file:`/modules/trusted-firmware-m/tfm_boards/common/crypto_keys.c`
   * - TF-M built-in HUK (CRACEN)
     - MKEK
     - PSA built-in key (``CRACEN_BUILTIN_MKEK_ID``); no application label
     - :ncs-file:`/include/tfm/tfm_builtin_key_ids.h`
   * - :ref:`Trusted Storage <trusted_storage_readme>` AEAD
     - MKEK on KMU devices; KDR without KMU
     - PSA storage UID (binary)
     - Trusted Storage backend
   * - :ref:`Secure Storage <secure_storage>` ITS transform
     - MKEK on KMU devices; KDR without KMU
     - PSA storage UID (binary)
     - Secure Storage subsystem
   * - Application-specific encryption key
     - MKEK on KMU devices; KDR without KMU
     - Application-defined label; the :ref:`Hardware unique key sample <hw_unique_key_usage>` uses ``sizeof(key_label) - 1`` (no terminating NUL)
     - :ref:`Hardware unique key sample <hw_unique_key_usage>`

.. _hw_unique_key_tfm_derivation:

Deriving keys with TF-M
-----------------------

When :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled, the non-secure application cannot call :c:func:`hw_unique_key_derive_key` directly.
Instead, use the PSA Crypto key derivation API with TF-M built-in keys, as shown in the :ref:`Hardware unique key sample <hw_unique_key_usage>`.
The sample selects HKDF on CryptoCell devices and SP800-108 CMAC on CRACEN devices.

Samples using the library
*************************

The following |NCS| samples demonstrate HUK provisioning or derivation:

* :ref:`Hardware unique key <hw_unique_key_usage>` - Derives a key from a HUK and encrypts data.
* :ref:`Crypto persistent key usage <crypto_persistent_key>` - Uses HUK-related configuration with persistent PSA keys.
* :ref:`TF-M Provisioning image <provisioning_image>` - Example of writing HUKs to the KMU during manufacturing.

Dependencies
************

This library depends on :kconfig:option:`CONFIG_NRF_SECURITY`.

On **nRF54L Series devices** (typical KMU configuration), it also depends on:

* CRACEN PSA driver (:kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN`)
* KMU

On **nRF91 Series and nRF5340 devices**, it depends on:

* :ref:`CC3xx platform libraries <nrf_cc3xx_platform_readme>`
* KMU

On **devices without a KMU**, it depends on:

* :ref:`CC3xx platform libraries <nrf_cc3xx_platform_readme>`
* ACL-backed flash for KDR material, :kconfig:option:`CONFIG_HW_UNIQUE_KEY_LOAD`, and bootloader support (see the KDR row in the device support table)

API documentation
*****************

| Header file: :ncs-file:`/include/hw_unique_key.h`
| Source files: :file:`lib/hw_unique_key/`

.. doxygengroup:: hw_unique_key
