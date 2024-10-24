.. _memory_storage:

Enabling Zephyr Memory Storage
##############################

.. contents::
   :local:
   :depth: 2

For nRF54L Series, you should use the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>`.
ZMS utilizes a flexible data management system that reduces write and erase cycles, extending the lifespan of non-volatile memory.

.. note::

  If you are using the :ref:`Non-Volatile Storage (NVS) <zephyr:nvs_api>`, ensure to switch to the ZMS solution.

Enabling the ZMS module
***********************

You can enable the ZMS module for your application in two ways - directly through ZMS or through the :ref:`Settings subsystem <zephyr:settings_api>`.
Additionally, you can optimize the application performance using caching mechanism:

.. tabs::

   .. group-tab:: Using storage directly through ZMS

      1. Enable the :kconfig:option:`CONFIG_ZMS` Kconfig option in your project's configuration file.
      #. Optionally, enable the lookup cache to increase the application performance:

        a. Enable the :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE` Kconfig option.
        #. Set the lookup cache size in :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE_SIZE` depending on your application needs.

   .. group-tab:: Using storage through Settings subsystem

      1. Enable the :kconfig:option:`CONFIG_ZMS` Kconfig option in your project's configuration file.
      #. Enable ZMS within the Settings subsystem with the :kconfig:option:`CONFIG_SETTINGS_ZMS` Kconfig option.
      #. Optionally, enable the lookup cache to increase the application performance:

        a. Enable the :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE` Kconfig option.
        #. Set the lookup cache size in :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE_SIZE` depending on your application needs.
        #. Ensure the lookup cache is configured to support the Setting subsystem by enabling the :kconfig:option:`CONFIG_ZMS_LOOKUP_CACHE_FOR_SETTINGS` Kconfig option.

      #. To further enhance the performance, enable the :kconfig:option:`CONFIG_SETTINGS_ZMS_NAME_CACHE` Kconfig option, and configure its size with :kconfig:option:`CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE` according to your application needs.

Optimizing ZMS in your application
**********************************

When integrating ZMS through the Settings subsystem in your application, you might need to optimize its performance.
The following sections include the best practices to enhance ZMS performance.

Sector size and count
=====================

To ensure optimal performance from ZMS, consider the following:

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
