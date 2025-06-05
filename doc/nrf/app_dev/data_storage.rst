.. _data_storage:
.. _memory_storage:

Data storage in the |NCS|
#########################

.. contents::
   :local:
   :depth: 2

The |NCS| offers multiple solutions for storing data persistently in non-volatile memory.

This page provides an overview of the available storage solutions to help you make an informed choice for your application's data storage needs.

For information about storing cryptographic keys, see :ref:`key_storage`.

General recommendations
***********************

For general data storage needs, Nordic Semiconductor recommends starting with the following options:

* NVMC - For applications requiring direct access to non-volatile memory.
* RRAMC - For applications requiring advanced RRAM memory management.
* NVS - For most applications requiring simple persistent storage.
  Stores data as values identified by a 16-bit integer ID.
* ZMS - Recommended for nRF54H and nRF54L Series devices, instead of NVS.
  Stores data as values identified by a 32-bit integer ID.
* File Systems - For applications requiring file-based data organization.
* Settings - For storing configuration data and runtime state.
  Organizes data in a tree structure, with values identified by a string key.
* PSA Protected Storage - For sensitive data.

Choose the storage mechanism that best matches your security requirements, performance needs, and platform capabilities.

Storage features
****************

This section describes the key features that are provided by storage mechanisms in the |NCS|.

See the following table for an overview of the storage features supported by each storage alternative.
For definitions of the storage features, see `Feature descriptions`_.

.. list-table:: Storage features comparison
   :widths: auto
   :header-rows: 1

   * - Storage Alternative
     - Partitioning
     - Integrity
     - Isolation
     - Authentication and encryption
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
   * - :ref:`RRAMC<data_storage_rramc>`
     - Yes
     - Yes
     - No
     - No
   * - :ref:`ZMS<data_storage_zms>`
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
     - Yes
     - No
     - No
   * - :ref:`PSA Protected Storage with TF-M <data_storage_psa_protected>`
     - Yes
     - Yes
     - Yes
     - Yes
   * - :ref:`PSA Protected Storage without TF-M <data_storage_psa_protected>`
     - Yes
     - Yes
     - No
     - Yes

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
This is typically implemented using Cyclic Redundancy Check (CRC).

The integrity can be inherited between a subsystem and its backends.
For example, the Settings subsystem inherits the integrity from its backend.

Isolation
---------

Isolation provides security separation between the Non-Secure Processing Environment (NSPE) and Secure Processing Environment (SPE) as part of the Arm Platform Security Architecture (PSA).
This feature is available when using :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`.

Authentication and encryption
-----------------------------

The authentication and encryption feature provides additional data-at-rest protection for stored data.
This adds confidentiality to prevent unauthorized access to sensitive information stored in non-volatile memory.

.. _data_storage_mechanisms:

Storage mechanisms
******************

The |NCS| provides several storage mechanisms for storing data persistently in non-volatile memory.
Each alternative offers different features and trade-offs in terms of security, performance, ease of use, and code and flash size.

.. _data_storage_nvmc:

NVMC
====

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
   For simple data storage needs, consider using :ref:`NVS <data_storage_nvs>` or :ref:`ZMS <data_storage_zms>` instead.

.. figure:: /images/data_storage_nvmc.svg
   :alt: Data storage using NVMC

   Data storage using NVMC

When using NVMC directly, manually ensure that the following requirements are met:

* Your writes do not conflict with other subsystems.
* You stay within designated memory regions.
* You handle error conditions appropriately.

To use NVMC safely, consider the following points:

* Create a custom partition for your memory usage.
* Keep all NVMC writes within this designated region.
* Coordinate with other subsystems that may use non-volatile memory.

Since NVMC allows writing to any part of non-volatile memory, it can easily overwrite memory used by other subsystems if not used carefully.

.. _data_storage_rramc:

RRAMC
=====

.. note::
   RRAMC is only available on the `nRF54L Series <nRF54L15 RRAMC_>`_ and should not be used directly, but through :ref:`ZMS <data_storage_zms>`.

.. list-table:: RRAMC features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **RRAMC**
     - Yes
     - Yes
     - No
     - No

The Resistive Random Access Memory Controller (RRAMC) provides advanced functionality for managing RRAM memory, including the secure information configuration region (SICR) and user information configuration registers (UICR).

.. figure:: /images/data_storage_rramc.svg
   :alt: Data storage using RRAMC

   Data storage using RRAMC

This option offers the following features:

* Error correction - Built-in ECC (Error Correction Code) can detect and correct up to two bit errors per 128-bit word line.
* Power management - Automatic standby or power-down modes with configurable latency.
* Region protection - `Configurable memory regions <nRF54L15 REGION.CONFIG_>`_ with read, write, and execute permissions, among others.
* Write-once support - Optional write-once protection for memory regions.
* Buffered writes - Support for fast buffered writes to contiguous memory regions.
* Power failure protection - Built-in power failure comparator to prevent writes during low voltage conditions.

The RRAMC provides several protection mechanisms, including:

* Immutable boot region - Configurable read-only boot region starting at address ``0x00000000``.
* One-time programmable protection - Hardware-fixed to write-once for UICR.
  Other regions can optionally be configured to be write-once.
* Erase protection - Ability to block ``ERASEALL`` operations.

When using RRAMC directly, manually ensure that the following requirements are met:

* Your writes do not conflict with other subsystems.
* You stay within designated memory regions.
* You handle error conditions appropriately.

For more information, see the `nRF54L Series SoC datasheets <nRF54L Series menu_>`_.

.. _data_storage_nvs:

NVS
===

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
NVS stores data as values identified by a 16-bit integer ID in non-volatile memory using a FIFO-managed circular buffer.

.. figure:: /images/data_storage_nvs.svg
   :alt: Data storage using NVS

   Data storage using NVS

This option offers the following features:

* Simple API - Basic functionality requires only three functions: ``nvs_mount()``, ``nvs_write()``, and ``nvs_read()``.
* Automatic integrity - Uses CRC to verify data integrity automatically.
* Wear leveling - Distributes writes across flash sectors to extend memory lifetime.
* Atomic operations - Ensures data consistency during power loss.
* Settings integration - Can be used directly or through the :ref:`Settings subsystem <zephyr:settings_api>`.

NVS automatically manages metadata for stored elements, including the following components:

* Element ID
* Data offset in the sector
* Data length
* CRC for integrity checking

During initialization, NVS verifies all stored data and ignores any data with missing or incorrect metadata.

Configuration
-------------

To use NVS, enable the following Kconfig options:

* :kconfig:option:`CONFIG_NVS`
* :kconfig:option:`CONFIG_FLASH`
* :kconfig:option:`CONFIG_FLASH_MAP`

Usage example
-------------

See Zephyr's :zephyr:code-sample:`nvs` sample for an example of how to use NVS.

.. _data_storage_zms:

ZMS
===

.. note::
   Use ZMS with the nRF54L and nRF54H Series devices as it works best with :ref:`RRAMC <data_storage_rramc>`.

.. list-table:: ZMS features
   :widths: auto
   :header-rows: 1

   * - Feature
     - Partitioning
     - Integrity
     - Isolation
     - Encryption
   * - **ZMS**
     - Yes
     - Yes
     - No
     - No

The :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` is the recommended storage solution for the nRF54L and nRF54H Series.
ZMS uses a flexible data management system that reduces write and erase cycles, extending the lifespan of non-volatile memory.
ZMS stores data as values identified by a 32-bit integer ID.

.. figure:: /images/data_storage_zms.svg
   :alt: Data storage using ZMS

   Data storage using ZMS

This option offers the following features:

* Simple API - Basic functionality requires only three functions: ``zms_mount()``, ``zms_write()``, and ``zms_read()``.
* Automatic integrity - Uses CRC to verify data integrity automatically.
* Wear leveling - Distributes writes across flash sectors to extend memory lifespan.
* Flexible data management - Efficient handling of data storage and retrieval.
* Cache support - Optional lookup cache for improved performance.
* Atomic operations - Ensures data consistency during power loss.
* Settings integration - Can be used directly or through the :ref:`Settings subsystem <zephyr:settings_api>`.

Configuration
-------------

To use ZMS, enable the :kconfig:option:`CONFIG_ZMS` Kconfig option.

For more information, see also :ref:`zms_memory_storage` in the |NCS| documentation and :ref:`ZMS documentation in Zephyr <zephyr:zms_api>`.

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

The following file systems are supported:

* FAT Filesystem - Cross-platform compatible format for data sharing.
* LittleFS - Power loss-resilient filesystem optimized for microcontrollers.

Configuration
-------------

To use VFS, enable the :kconfig:option:`CONFIG_FILE_SYSTEM` Kconfig option.

File systems require storage media configuration (flash, SD card) and the appropriate Kconfig option to be enabled:

* :kconfig:option:`CONFIG_FAT_FILESYSTEM_ELM` - Enable FAT filesystem
* :kconfig:option:`CONFIG_FILE_SYSTEM_LITTLEFS` - Enable LittleFS

Usage examples
--------------

For usage examples, see the following Zephyr samples:

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
     - Yes
     - No
     - No

The :ref:`Settings <zephyr:settings_api>` subsystem provides a method for organizing and storing configuration data and runtime state.
It gives modules a way to store per-device configuration, which is organized in a tree structure.
While NVS and ZMS store data as values identified by an integer ID, Settings stores data values identified by a string key.

.. figure:: /images/data_storage_settings.svg
   :alt: Data storage using Settings

   Data storage using Settings

Key features:

* Hierarchical organization - Settings are organized in a tree structure.
* Automatic loading - Call ``settings_load()`` to restore all settings after reboot.
* Backend flexibility - Settings can use different storage backends.

The Settings subsystem does not write to non-volatile memory directly.
Instead, it uses other subsystems as backends, with :ref:`data_storage_nvs` being the recommended choice for most devices and :ref:`data_storage_zms` for nRF54L Series and nRF54H20 devices.

You can use the ``settings_load_one()`` function to load only the value defined by the tree name given in the arguments.
This is a very fast way to get a value from the storage if you know its tree name.
It is particularly suitable for use with ZMS because it does not need to load all settings before getting the right one.

Settings are used by various subsystems, including :ref:`Bluetooth host stack <zephyr:bluetooth-persistent-storage>` and :ref:`Matter <ug_matter>`.

Configuration
-------------

To use Settings, enable the :kconfig:option:`CONFIG_SETTINGS` Kconfig option.

Depending on the backend you want to use, enable the appropriate Kconfig options:

* For the NVS backend:

  * :kconfig:option:`CONFIG_SETTINGS_NVS`
  * :kconfig:option:`CONFIG_NVS`
  * :kconfig:option:`CONFIG_FLASH_MAP`

* For the ZMS backend:

  * :kconfig:option:`CONFIG_SETTINGS_ZMS`
  * :kconfig:option:`CONFIG_ZMS`

* For the File System backend:

  * :kconfig:option:`CONFIG_SETTINGS_FILE`
  * :kconfig:option:`CONFIG_FILE_SYSTEM` - with the appropriate file system enabled, as mentioned in :ref:`data_storage_filesystems`

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
   * - **PSA Protected Storage with TF-M**
     - Yes
     - Yes
     - Yes
     - Yes
   * - **PSA Protected Storage without TF-M**
     - Yes
     - Yes
     - No
     - Yes

The PSA Protected Storage API provides functionality for writing and reading data from non-volatile memory with enhanced security features.
This storage solution is part of the :ref:`ug_psa_certified_api_overview_secstorage`, which in turn is part of the :ref:`Platform Security Architecture (PSA) <ug_psa_certified_api_overview>` framework.

.. tabs::

   .. tab:: PSA Protected Storage with TF-M

      .. figure:: /images/data_storage_psa_tfm.svg
        :alt: Data storage using PSA Protected Storage

        Data storage using PSA Protected Storage

   .. tab:: PSA Protected Storage without TF-M

      .. figure:: /images/data_storage_psa_no_tfm.svg
        :alt: Data storage using PSA Protected Storage

        Data storage using PSA Protected Storage

Key features:

* Authentication and encryption - Data is authenticated and encrypted at rest for confidentiality.
* Isolation - Provides security separation between NSPE and SPE.
* Integrity - Automatic integrity verification.
* Write-once support - Supports write-once flags for immutable data.
* PSA compliance - Compliant with PSA Certified standards.

.. note::
   PSA Protected Storage does not currently support storing data to external flash.
   The data stored through the PSA Protected Storage API will be written to the same location as the PSA Internal Trusted Storage.

PSA Protected Storage and TF-M
------------------------------

PSA Protected Storage is provided by :ref:`TF-M <ug_tfm>` when TF-M is used.
When using TF-M, PSA Protected Storage is an :ref:`Application RoT Service <ug_tfm_architecture_rot_services_application>` that runs in the :ref:`Secure Processing Environment (SPE) <app_boards_spe_nspe>`.
It is available to both NSPE and SPE, but data stored from one environment is not accessible from the other.

When to use PSA Protected Storage
---------------------------------

Consider using PSA Protected Storage in the following cases:

* You need encryption and authentication for your data at rest.
* You require PSA Certified compliant storage.
* You are building security-critical applications.

When comparing PSA Protected Storage with :ref:`data_storage_nvs` or :ref:`data_storage_zms`, consider the following:

* Choose PSA Protected Storage when you need:

  * Encrypted data storage
  * Security isolation between environments (when using TF-M)
  * PSA Certified compliance
  * Write-once functionality (only enforced by software)

* Choose NVS or ZMS when you need:

  * Simple key-value storage
  * Lower overhead
  * No security requirement for data stored directly using NVS or ZMS

Configuration
-------------

When using :ref:`TF-M <ug_tfm>`, PSA Protected Storage is enabled when its requirements are met and the :kconfig:option:`CONFIG_TFM_PARTITION_PROTECTED_STORAGE` Kconfig option is enabled.

When not using :ref:`TF-M <ug_tfm>`, you must enable the following Kconfig options:

* :kconfig:option:`CONFIG_TRUSTED_STORAGE`
* :kconfig:option:`CONFIG_PSA_PROTECTED_STORAGE`

Usage example
-------------

For a usage example, see Zephyr's :zephyr:code-sample:`psa_protected_storage` sample.
