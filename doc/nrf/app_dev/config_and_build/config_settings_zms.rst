.. _config_settings_zms:

Configuring Zephyr Memory Storage
#################################

.. contents::
   :local:
   :depth: 2

Zephyr's :ref:`Settings subsystem <zephyr:settings_api>` provides a structured way to store configuration data in non-volatile memory.
There are two modules available:

* :ref:`Non-Volatile Storage (NVS) <zephyr:nvs_api>` module offers an approach with wear leveling and redundancy to protect data integrity.
* The newer :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` alternative enhances performance and storage efficiency, utilizing a more flexible data management system that reduces write and erase cycles, extending the lifespan of flash memory.
  The solution has been adopted as the default storage system for devices equipped with resistive (RRAM) or magnetic (MRAM) non-volatile memory technologies.

Depending on the device you are working with, the storage solution will differ:

.. list-table::
   :header-rows: 1

   * - Device
     - Supported module
   * - nRF54L Series' devices
     - Zephyr Memory Storage (ZMS)
   * - Other devices
     - Non-Volatile Storage (NVS)

Enabling the ZMS module
***********************

You can enable the ZMS module for your application in two ways - directly through ZMS or through the Settings subsystem.
Additionally, you can optimze the application performance using caching mechanism:

.. tabs::

   .. group-tab:: Using storage directly through ZMS

      1. Enable the :kconfig:option:`CONFIG_ZMS` Kconfig option in your project's configuration file.
      #. Optionally, enable the lookup cache to increase the application performance:

        a. Enable the :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE` Kconfig option.
        #. Set the lookup cache size in :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE_SIZE` to ``512``.

   .. group-tab:: Using storage through Settings subsystem

      1. Enable the :kconfig:option:`CONFIG_ZMS` configuration.
      #. Enable ZMS within the Settings subsystem with the :kconfig:option:`CONFIG_SETTINGS_ZMS` Kconfig option.
      #. Optionally, enable the lookup cache to increase the application performance:

        a. Enable the :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE` Kconfig option.
        #. Set the lookup cache size in :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE_SIZE` to ``512``.
        #. Ensure the lookup cache is configured to support the Setting subsystem by enabling the :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE_FOR_SETTINGS` Kconfig option.

Optimizing ZMS in your application
**********************************

When integrating ZMS through the Settings subsystem in your application, you might need to optimze its performance, especially when handling a significant number of entries.
The following sections include the best practices to enhance ZMS performance.

Sector size and count
=====================

To ensure optimal performance from ZMS, you should properly dimension both the total storage partition size and the sector size:

* Storage partition sizing - The total storage partition size and the sector size should be carefully dimensioned to optimize ZMS performance.
  For detailed instructions on managing free space effectively within ZMS, refer to the :ref:`ZMS <zephyr:zms_api>` documentation.
  You should choose a storage partition capable of holding double the size of the key-value pairs expected to be written.
* Entry management - In subsystems like Settings, all path-value pairs are split into two ZMS entries (ATEs).
  You should include the extra header needed by these entries in your total storage space calculations.
* Optimizing data storage - Storing small data directly in the ZMS entry headers can significantly enhance performance.
  For instance, in the Settings subsystem, using a path name of 8 bytes or less can speed up read and write operations.

Dimensioning cache
==================

Proper cache sizing is critical for maintaining high performance and efficient memory usage in ZMS:

* Cache size determination - Ideally, the cache size should match the number of different entries that could be written to the storage.
  This ensures efficient data retrieval and storage operations.
* Memory considerations - Each cache entry consumes an additional 8 bytes of RAM.
  Therefore, you should choose the cache size to balance performance with memory usage effectively.
* Settings subsystem specifics - When using ZMS through the Settings subsystem, remember that each entry translates into two ZMS entries.
  Consequently, the optimal cache size should be twice the number of Settings entries to accommodate this division effectively.
