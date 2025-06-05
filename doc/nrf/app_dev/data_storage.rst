.. _data_storage:

Data storage in the |NCS|
#########################

.. contents::
   :local:
   :depth: 2

The |NCS| offers multiple storage solutions for storing data persistently in non-volatile memory.

This page provides an overview of the available storage alternatives to help you make an informed choice for your application's data storage needs.

For information about storing cryptographic keys, see :ref:`key_storage`.

General recommendations
***********************

For general data storage needs, Nordic Semiconductor recommends starting with the following options:

* NVS - For most applications requiring simple persistent storage.
* File Systems - For applications requiring file-based data organization.
* Settings - For configuration data and runtime state management.
* PSA Protected Storage - For security-sensitive data when using TF-M.

Choose the storage mechanism that best matches your security requirements, performance needs, and platform capabilities.

Storage features
****************

This section describes the key features that can be provided by storage mechanisms in the |NCS|.

See the following table for an overview of the storage features supported by each storage alternative.
For definitions, see `Feature descriptions`_.

.. list-table:: Storage features comparison
   :widths: auto
   :header-rows: 1

   * - Storage Alternative
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - :ref:`NVMC <data_storage_nvmc>`
     - No
     - No
     - No
     - No
   * - :ref:`NVS <data_storage_nvs>`
     - Yes
     - Yes
     - No
     - No
   * - :ref:`File Systems <data_storage_filesystems>`
     - Yes
     - Yes
     - No
     - No
   * - :ref:`Settings <data_storage_settings>`
     - Yes
     - Yes*
     - No
     - No
   * - :ref:`PSA Protected Storage <data_storage_psa_protected>`
     - Yes
     - Yes
     - Yes
     - Yes

\* The Settings subsystem can use different backends. If the backend has integrity, Settings also does.

Feature descriptions
====================

See the following sections for descriptions of the storage features from the table above.

Partitioning
------------

Partitioning prevents storage subsystems from overwriting each other's data by giving them ownership over different non-volatile memory regions.
In the |NCS|, you can use either :ref:`Devicetree fixed flash partitions <zephyr:flash_map_api>` or the :ref:`Partition Manager <partition_manager>` to manage non-volatile memory partitions.

Integrity
---------

Data integrity ensures that the data you read is the same as the data you wrote.
This is typically implemented using Cyclic Redundancy Check (CRC) or cryptographic hash functions like SHA-2.

Isolation
---------

Isolation provides security separation between the Non-Secure Processing Environment (NSPE) and Secure Processing Environment (SPE) as part of the Arm Platform Security Architecture (PSA).
This feature is available when using :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`.

Encryption
----------

Encryption provides additional data-at-rest protection by encrypting stored data.
This adds confidentiality to prevent unauthorized access to sensitive information stored in non-volatile memory.

.. _data_storage_alternatives:

Storage alternatives
********************

The |NCS| provides several storage mechanisms for storing data persistently in non-volatile memory.
Each alternative offers different features and trade-offs in terms of security, performance, and ease of use.

.. _data_storage_nvmc:

NVMC (Non-Volatile Memory Controller)
=====================================

.. list-table:: NVMC features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **NVMC**
     - No
     - No
     - No
     - No

The Non-Volatile Memory Controller (NVMC) provides the most basic method for writing directly to non-volatile memory.
The |NCS| provides the `NVMC driver through nrfx <nrfx NVMC driver_>`_ as a helper for this functionality.

.. note::
   Nordic Semiconductor does not recommend this method for persistent storage, as it places significant responsibility on the developer.
   For simple data storage needs, consider using :ref:`NVS <data_storage_nvs>` instead.

.. figure:: /images/data_storage_nvmc.svg
   :alt: Data storage using NVMC

   Data storage using NVMC

When using the NVMC directly, you must manually ensure that:

* Your writes do not conflict with other subsystems
* You stay within designated memory regions
* You handle error conditions appropriately

To use NVMC safely:

1. Create a custom partition for your memory usage
2. Keep all NVMC writes within this designated region
3. Coordinate with other subsystems that may use non-volatile memory

Since NVMC allows writing to any part of non-volatile memory, it can easily overwrite memory used by other subsystems if not used carefully.

.. _data_storage_nvs:

NVS (Non-Volatile Storage)
==========================

.. list-table:: NVS features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **NVS**
     - Yes
     - Yes
     - No
     - No

The :ref:`Non-Volatile Storage (NVS) <zephyr:nvs_api>` subsystem is Zephyr's default persistent storage solution.
NVS stores data as ID-data pairs in non-volatile memory using a FIFO-managed circular buffer.

.. figure:: /images/data_storage_nvs.svg
   :alt: Data storage using NVS

   Data storage using NVS

This option offers the following features:

* Simple API - Basic functionality requires only three functions: ``nvs_mount()``, ``nvs_write()``, and ``nvs_read()``.
* Automatic integrity - Uses CRC to verify data integrity automatically.
* Wear leveling - Distributes writes across flash sectors to extend memory lifetime.
* Atomic operations - Ensures data consistency during power loss.

NVS automatically manages metadata for stored elements, including the following components:

* Element ID
* Data offset in the sector
* Data length
* CRC for integrity checking

During initialization, NVS verifies all stored data and ignores any data with missing or incorrect metadata.

Configuration
-------------

To enable NVS, set the following Kconfig options:

* :kconfig:option:`CONFIG_NVS`
* :kconfig:option:`CONFIG_FLASH`
* :kconfig:option:`CONFIG_FLASH_MAP`

Usage example
-------------

See Zephyr's :zephyr:code-sample:`nvs` sample for an example of how to use NVS.

.. _data_storage_filesystems:

File Systems
============

.. list-table:: File Systems features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **File Systems**
     - Yes
     - Yes
     - No
     - No

Zephyr's Virtual Filesystem Switch (VFS) allows applications to mount multiple file systems at different mount points using the :ref:`File System API <zephyr:file_system_api>`.

.. figure:: /images/data_storage_file_system.svg
   :alt: Data storage using file systems

   Data storage using file systems

Two file systems are supported:

* FAT Filesystem - Cross-platform compatible format for data sharing.
* LittleFS - Power-loss resilient filesystem optimized for microcontrollers.

Configuration
-------------

File systems require storage media configuration (flash, SD card) and the appropriate file system driver:

* :kconfig:option:`CONFIG_FILE_SYSTEM` - Enable VFS support
* :kconfig:option:`CONFIG_FAT_FILESYSTEM_ELM` - Enable FAT filesystem
* :kconfig:option:`CONFIG_FILE_SYSTEM_LITTLEFS` - Enable LittleFS

Usage examples
--------------

For usage examples, see the following pages in Zephyr:

* :zephyr:code-sample:`fs`
* :zephyr:code-sample:`shell-fs`
* :zephyr:code-sample:`usb-mass`

.. _data_storage_settings:

Settings
========

.. list-table:: Settings features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **Settings**
     - Yes
     - Yes*
     - No
     - No

\* The Settings subsystem can use different backends. If the backend has integrity, Settings also does.

The :ref:`Settings <zephyr:settings_api>` subsystem provides a structured method for organizing and storing configuration data and runtime state.
It gives modules a way to store persistent per-device configuration that survives reboots.

.. figure:: /images/data_storage_settings.svg
   :alt: Data storage using Settings

   Data storage using Settings

Key features:

* **Hierarchical organization**: Settings are organized in a tree structure
* **Automatic loading**: Call ``settings_load()`` to restore all settings after reboot
* **Backend flexibility**: Can use different storage backends (:ref:`data_storage_nvs` is recommended)

The Settings subsystem does not write to non-volatile memory directly.
Instead, it uses other subsystems as backends, with :ref:`data_storage_nvs` being the recommended choice.

Settings are used by various subsystems, including :ref:`Bluetooth host stack <zephyr:bluetooth-persistent-storage>` and :ref:`Matter <ug_matter>`.

Configuration
-------------

To enable Settings with the NVS backend:

* :kconfig:option:`CONFIG_SETTINGS`
* :kconfig:option:`CONFIG_SETTINGS_NVS`

Usage example
-------------

See Zephyr's :zephyr:code-sample:`settings` sample for an example of how to use the Settings subsystem.
You can also check the examples in the :ref:`Settings subsystem <zephyr:settings_api>` documentation.

.. _data_storage_psa_protected:

PSA Protected Storage
=====================

.. list-table:: PSA Protected Storage features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **PSA Protected Storage**
     - Yes
     - Yes
     - Yes
     - Yes

The PSA Protected Storage API provides functionality for writing and reading data from non-volatile memory with enhanced security features.
This storage solution is part of the :ref:`ug_psa_certified_api_overview_secstorage` implementation, which in turn is part of the :ref:`Platform Security Architecture (PSA) <ug_psa_certified_api_overview>`.

.. figure:: /images/data_storage_psa.svg
   :alt: Data storage using PSA Protected Storage

   Data storage using PSA Protected Storage

Key features:

* **Encryption**: Data is encrypted at rest for confidentiality
* **Isolation**: Provides security separation between NSPE and SPE
* **Integrity**: Automatic integrity verification
* **Write-once support**: Supports write-once flags for immutable data
* **PSA compliance**: Compliant with PSA Certified standards

PSA Protected Storage is an :ref:`Application RoT Service <ug_tfm_architecture_rot_services_application>` that runs in the :ref:`Secure Processing Environment (SPE) <app_boards_spe_nspe>`.
It is available to both NSPE and SPE, but data stored from one environment is not accessible from the other.

When to use PSA Protected Storage
---------------------------------

Consider using PSA Protected Storage in the following cases:

* You need data encrypted at rest.
* You require PSA Certified compliant storage.
* You need write-once functionality for immutable data.
* You are building security-critical applications.

When comparing PSA Protected Storage with :ref:`data_storage_nvs`, consider the following:

* Choose PSA Protected Storage when you need:

  * Encrypted data storage
  * Security isolation between environments
  * PSA Certified compliance
  * Write-once functionality

* Choose NVS when you need:

  * Simple key-value storage
  * Lower overhead
  * Broader platform compatibility
  * No security isolation requirements

Configuration
-------------

PSA Protected Storage requires :ref:`TF-M <ug_tfm>` to be enabled.
The necessary Kconfig options are typically enabled automatically when TF-M is configured.

Usage example
-------------

For a usage example, see Zephyr's :zephyr:code-sample:`psa_protected_storage` sample and the :ref:`tfm_secure_peripheral_partition` sample in the |NCS|.
