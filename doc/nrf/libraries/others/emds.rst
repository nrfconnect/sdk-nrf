.. _emds_readme:

Emergency data storage
######################

.. contents::
    :local:
    :depth: 2

Overview
********
The emergency data storage (EMDS) library provides persistent storage functionality designed to prevent the wear and tear of the flash memory.
Its intended use is for storing of data undergoing frequent changes during runtime.

Implementation
==============
The :kconfig:option:`CONFIG_EMDS` Kconfig option enables the emergency data storage.

The application must initialize the pre-allocated flash area by using the :c:func:`emds_init` function.
The :kconfig:option:`CONFIG_EMDS_SECTOR_COUNT` option defines how many sectors should be used to store data.

The allocated storage space must be larger than the combined data stored by the application.
Allocating a larger flash area will demand more resources, but also increase the life expectancy of the flash.
The chosen size should reflect the amount of data stored, the available flash resources, and how the application calls the :c:func:`emds_store` function.
In general, it should not be necessary to allocate a large flash area, since only a limited set of data should be stored to ensure swift completion of writing the flash on shutdown.

The memory location that is going to be stored must be added on initialization.
All memory areas must be provided through entries containing an ID, data pointer, and data length, using the :c:func:`emds_entry_add` function and the :c:macro:`EMDS_STATIC_ENTRY_DEFINE` macro.
Entries to be stored when the emergency data storage is triggered need their own unique IDs that are not changed after a reboot.

When all entries are added, the :c:func:`emds_load` function restores the entries into the memory areas from the flash.

After restoring the previous data, the application must run the :c:func:`emds_prepare` function to prepare the flash area for receiving new entries.
If the remaining empty flash area is smaller than the required data size, the flash area will be automatically erased to increase the available flash area.

The storage is done in deterministic time, so it is possible to know how long it takes to store all registered entries.
However, this is chip-dependent, so it is important to measure the time.
The `Nordic Semiconductor Infocenter`_ contains chip information and datasheet, and timing values can be found under the "Electrical specification" for the Non-volatile memory controller.
The following Kconfig options can be configured:

* :kconfig:option:`CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US`
* :kconfig:option:`CONFIG_EMDS_FLASH_TIME_ENTRY_OVERHEAD_US`
* :kconfig:option:`CONFIG_EMDS_FLASH_TIME_BASE_OVERHEAD_US`

When configuring these values, consider the time for erase when doing garbage collection in NVS.
If partial erase is not enabled or supported, the time of a complete sector erase has to be included in the :kconfig:option:`CONFIG_EMDS_FLASH_TIME_BASE_OVERHEAD_US`.
When partial erase is enabled and supported by the hardware, include the time it takes for the scheduler to trigger, which is depending on the time defined in :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE_MS`.
When changing the :kconfig:option:`CONFIG_EMDS_FLASH_TIME_BASE_OVERHEAD_US` option, it is important that the worst time is considered.

The application must call the :c:func:`emds_store` function to store all entries.
This can only be done once, before the :c:func:`emds_prepare` function must be called again.
When invoked, the :c:func:`emds_store` function stores all the registered entries.
Invocation of this call should be performed when the application detects loss of power, or when a reboot is triggered.

.. note::
    Before calling the :c:func:`emds_store` function, the application should try shutting down the application-specific features that consume a lot of power.
    Shutting down these features may prolong the time the CPU is alive, and improve the storage time.
    For example, if Bluetooth is used, disabling Bluetooth before shutdown will save power, and stopping the MPSL scheduler will shorten the total time required to complete the store operation.

The :c:func:`emds_is_ready` function can be called to check if EMDS is prepared to store the data.

Once the data storage has completed, a callback is called if provided in :c:func:`emds_init`.
This callback notifies the application that the data storage has completed, and can be used to reboot the CPU or execute another function that is needed.

After completion of :c:func:`emds_store`, the :c:func:`emds_is_ready` function call will return error, since it can no longer guarantee that the data will fit into the flash area.

The above described process is summarized in a message sequence diagram.

.. msc::
    hscale = "1.3";
    Application, EMDS;
    --- [ label = "Application initialization started" ];
    Application=>EMDS         [ label = "emds_init(emds_store_cb_t)" ];
    --- [ label = "Initialization of all functionality that does emds_entry_add()" ];
    Application=>EMDS         [ label = "emds_entry_add(1)" ];
    Application=>EMDS         [ label = "emds_entry_add(2)" ];
    ...;
    Application=>EMDS         [ label = "emds_entry_add(n)" ];
    --- [ label = "All emds_entry_add() executed" ];
    Application=>EMDS         [ label = "emds_load()" ];
    Application=>EMDS         [ label = "emds_prepare()" ];
    --- [ label = "Application initialization ended" ];
    ...;
    Application->Application  [ label = "Interrupt calling emds_store()" ];
    Application=>EMDS         [ label = "emds_store()" ];
    Application<<=EMDS        [ label = "emds_store_cb_t callback" ];
    Application->Application [ label = "Reboot/halt" ];

Requirements
************
To prevent frequent writes to flash memory, the EMDS library can write data to flash only when the device is shutting down.
EMDS restores the application data to RAM at reboot.

EMDS can store data within a guaranteed time, based on the amount of data being stored.
EMDS can be used to store data in memory in situations of critical power shortage, for example before the device battery is depleted.
It is important that the hardware has the appropriate functionality to sustain power long enough for the storage to be completed before the power source is fully discharged.

Configuration
*************
To initialize the emergency data storage, complete the following steps:

1. Enable the :kconfig:option:`CONFIG_EMDS` Kconfig option.
#. Include the :file:`emds/emds.h` file in your :file:`main.c` file.
#. Create the callback function :c:func:`emds_store_cb_t` that can execute functions after storage has completed. This is optional.
#. Call the :c:func:`emds_init` function.
#. Add RAM areas that shall be loaded/stored through :c:func:`emds_entry_add` calls.
#. Call :c:func:`emds_load`.
#. Call :c:func:`emds_prepare`.
#. Create interrupt or other functionality that will call :c:func:`emds_store`.

.. _emds_readme_application_integration:

Application integration
***********************

When using EMDS in an application, you need to know the worst case scenario for how long power is required to be available.
This knowledge makes it possible for you to make good design choices ensuring enough backup power to reach this time requirement.

The easiest way of computing an estimate of the time required to store all entries, in a worst case scenario, is to call the :c:func:`emds_store_time_get` function.
This function returns a worst-case storage time estimate in microseconds (µs) for a given application.
For this to work, Kconfig options :kconfig:option:`CONFIG_EMDS_FLASH_TIME_BASE_OVERHEAD_US`, :kconfig:option:`CONFIG_EMDS_FLASH_TIME_ENTRY_OVERHEAD_US` and :kconfig:option:`CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US` need to be set as described in the `Implementation`_ section.
The :c:func:`emds_store_time_get` function estimates the required worst-case time to store :math:`n` entries using the following formula:

.. math::

   t_\text{store} = t_\text{base} + \sum_{i = 1}^n \left(t_\text{entry} + t_\text{word}\left(\left\lceil\frac{s_\text{ate}}{s_\text{block}}\right\rceil + \left\lceil\frac{s_i}{s_\text{block}}\right\rceil \right)\right)

where :math:`t_\text{base}` is the value specified by :kconfig:option:`CONFIG_EMDS_FLASH_TIME_BASE_OVERHEAD_US`, :math:`t_\text{entry}` is the value specified by :kconfig:option:`CONFIG_EMDS_FLASH_TIME_ENTRY_OVERHEAD_US` and :math:`t_\text{word}` is the value specified by :kconfig:option:`CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US`.
:math:`s_i` is the size of the :math:`i`\ th entry in bytes and :math:`s_\text{block}` is the number of bytes in one word of flash.
These can be found by looking at datasheets, driver documentation, and the configuration of the application.
:math:`s_\text{ate}` is the size of the allocation table entry used by the EMDS flash module, which is 8 B.

Example of time estimation
==========================

Using the formula from the previous section, you can estimate the time required to store all entries for the :ref:`bluetooth_mesh_light_lc` sample running on the nRF52840.
The following values can be inserted into the formula:

*  Set :math:`t_\text{base}` = 9000 µs.
   This is the worst case overhead when a store is triggered in the middle of an erase on nRF52840 with :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` enabled in the sample by default, and should be adjusted when using other configurations.
*  Set :math:`t_\text{entry}` = 300 µs and :math:`t_\text{word}` = 41 µs. *Note: These values are valid only for this specific chip and configuration, and should be computed for the specific configuration whenever using EMDS.*
*  The sample uses two entries, one for the RPL with 255 entries (:math:`s_i` = 2040 B) and one for the lightness state (:math:`s_i` = 3 B).
*  The flash write block size :math:`s_\text{block}` in this case is 4 B, and the ATE size :math:`s_\text{ate}` is 8 B.

This gives the following formula to compute estimated storage time:

.. math::
   \begin{aligned}
   t_\text{store} = 9000\text{ µs} &+ \left( 300\text{ µs} + 41\text{ µs} \times \left( \left\lceil\frac{8\text{ B}}{4\text{ B}}\right\rceil + \left\lceil\frac{2040\text{ B}}{4\text{ B}}\right\rceil \right) \right) \\
   &+ \left( 300\text{ µs} + 41\text{ µs} \times \left( \left\lceil\frac{8\text{ B}}{4\text{ B}}\right\rceil + \left\lceil\frac{3\text{ B}}{4\text{ B}}\right\rceil \right) \right) \\
   &= 30715\text{ µs}
   \end{aligned}

Calling the :c:func:`emds_store_time_get` function in the sample automatically computes the result of the formula and returns 30715.

Limitations
***********
    The power-fail comparator for the nRF528xx cannot be used with EMDS, as it will prevent the NVMC from performing write operations to flash.

Dependencies
************
The emergency data storage is dependent on these Kconfig options:

* :kconfig:option:`CONFIG_PARTITION_MANAGER_ENABLED`
* :kconfig:option:`CONFIG_FLASH_MAP`

API documentation
*****************

| Header file: :file:`include/emds/emds.h`
| Source file: :file:`subsys/emds/emds.c`

.. doxygengroup:: emds
    :project: nrf
    :members:
