.. _emds_readme:

Emergency data storage
######################

.. contents::
    :local:
    :depth: 2

The emergency data storage (EMDS) library provides persistent storage functionality designed to prevent the wear and tear of the flash memory or RRAM (persistent memory).
Its intended use is for storing of data undergoing frequent changes during runtime.

Implementation
**************
The :kconfig:option:`CONFIG_EMDS` Kconfig option enables the emergency data storage.

The EMDS uses the persistent memory in form of storage partitions, which can be either flash or RRAM.
You must align the storage partitions to the write block size, which is 4 bytes for flash and 16 bytes for RRAM.
The EMDS mandates at least two storage partitions to allow restoring of the last known copy of data.
Define the storage partitions in the devicetree file used for the application.
The devicetree file must contain two partitions with names ``emds_partition_0`` and ``emds_partition_1`` with start offset and size specified.

.. code-block:: devicetree

	partitions {
		emds_partition_0: partition@fe000 {
			label = "emds-0";
			reg = <0x000fe000 0x00001000>;
		};

		emds_partition_1: partition@ff000 {
			label = "emds-1";
			reg = <0x000ff000 0x00001000>;
		};
	};

Place the partitions in the persistent memory area of the device.
The exact location addresses depend on the specific device.
The exact partition sizes depend on the amount of data that the application needs to store.
It is recommended to have partitions with equal sizes.
Partitions must stay on the same location and with the same size for all application versions that use EMDS, to ensure compatibility when updating the application.
The application must initialize storage partitions by using the :c:func:`emds_init` function.

The partition storage space for each partition must be larger than the combined data stored by the application.
To find out how much storage space is needed, call the :c:func:`emds_store_size_get` function.
Additionally, consider the size of the metadata, which is 32 bytes for each snapshot.
Allocating a larger storage area will demand more resources, but also increase the life expectancy of the persistent memory.
The chosen size should reflect the amount of data stored, the available persistent memory and how the application calls the :c:func:`emds_store` function.

The memory location that is going to be stored must be added on initialization.
All memory areas must be provided through entries containing an ID, data pointer, and data length, using the :c:func:`emds_entry_add` function and the :c:macro:`EMDS_STATIC_ENTRY_DEFINE` macro.
Entries to be stored when the emergency data storage is triggered need their own unique IDs that are not changed after a reboot.
The entries are stored in the partition in form of snapshots.
The snapshot is a packed copy of all entries that are registered with the EMDS library for storage in the data area of the partition.
Each snapshot is described by the metadata that is stored in the metadata area of the partition.

After initialization is completed by registering all EMDS entries, the application calls the :c:func:`emds_load` function.
This starts a partition scanning procedure to find the latest snapshot.
As a part of loading procedure, the EMDS scans the metadata area of both partitions.
The :kconfig:option:`CONFIG_EMDS_SCANNING_FAILURES` Kconfig option configures the depth of the scanning procedure.
The depth is the number of times the scanning procedure can fail in a row before it stops, because most likely, there is no stored metadata left.
The scanning procedure is performed by reading the metadata area of the partition and checking the integrity of the metadata.
The procedure gathers the most recent snapshots (set by :kconfig:option:`CONFIG_EMDS_MAX_CANDIDATES`) from the partition metadata area.
After the procedure is completed, the EMDS checks data integrity on the gathered candidates, starting from the latest one stored on the partition.
If the data integrity checks pass, the entries are restored into the memory areas from the persistent memory.
If candidates are not found or the data integrity check does not pass, the function returns an error.
The :c:func:`emds_load` function returns an error code if it runs on the empty partitions.

After restoring the previous data, the application must call the :c:func:`emds_prepare` function to prepare the storage area for receiving new entries.
Initially, the EMDS library tries to allocate the storage area for the new entries in the partition that has the most recent snapshot to use the maximum partition capacity.
However, if the partition is full, the EMDS tries to allocate the storage area in the other partition.
If the other partition is full, the EMDS erases the entire partition to get space for a new snapshot, depending on the type of persistent memory.
Devices with flash memory require an explicit erase, whereas devices with RRAM do not need an explicit erase.
The EMDS library handles this internally.
The EMDS never erases the partition that has the most recent snapshot found during the partitions scanning procedure.

The storage is done in deterministic time, so it is possible to know how long it takes to store all registered entries.
However, this is chip-dependent, so it is important to measure the time.
Find timing values under the "Electrical specification" section for the non-volatile memory controller in the Product Specification for the relevant SoC or the SiP you are using.
For example, for the nRF52840 SiP, see the `nRF52840 Product Specification`_ page.
The data is stored by chunks of 16 bytes.
The storing time is determined by the chunk preparation time and the flash writing time, and depends on both the number of stored data bytes (both data and metadata) as well as the number of chunks.

The following (non-public) Kconfig options are needed for the time estimation:

* :kconfig:option:`CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US`
* :kconfig:option:`CONFIG_EMDS_CHUNK_PREPARATION_TIME_US`

You can tune these options to influence the estimation of the writing time (see :c:func:`emds_store_time_get`), but they do not change the actual time needed for storing the snapshot.
It is recommended to consider the worst case scenarios when adjusting these options.

The application must call the :c:func:`emds_store` function to store all entries.
This can only be done once, before the :c:func:`emds_load` and :c:func:`emds_prepare` functions must be called again.
When invoked, the :c:func:`emds_store` function stores all the registered entries.
Invocation of this call should be performed when the application detects loss of power, or when a reboot is triggered.

.. note::
    Before calling the :c:func:`emds_store` function, the application should try shutting down the application-specific features that consume a lot of power.
    Shutting down these features may prolong the time the CPU is alive, and improve the storage time.
    For example, if Bluetooth is used, disabling Bluetooth before shutdown will save power, and stopping the MPSL scheduler will shorten the total time required to complete the store operation.

The :c:func:`emds_is_ready` function can be called to check if EMDS is prepared to store the data.

Once the data storage has completed, a callback is called if provided in :c:func:`emds_init`.
This callback notifies the application that the data storage has completed, and can be used to reboot the CPU or execute another function that is needed.

After completion of :c:func:`emds_store`, the :c:func:`emds_is_ready` function call will return an error, because it can no longer guarantee that the data will fit into the persistent memory area.

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
To prevent frequent writes to persistent memory, the EMDS library can write data only when the device is shutting down.
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
To make this work, you need to determine and set the Kconfig options :kconfig:option:`CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US` and :kconfig:option:`CONFIG_EMDS_CHUNK_PREPARATION_TIME_US` as described in the `Implementation`_ section for your platform.
The :c:func:`emds_store_time_get` function estimates the required worst-case time to store :math:`n` entries using the following formula:

.. math::

   t_\text{store} = t_\text{word}\lceil\frac{s_\text{meta}}{s_\text{word}}\rceil + t_\text{word} \sum_{i = 1}^n (\left\lceil\frac{s_i}{s_\text{word}}\right\rceil) + t_\text{chunk} \sum_{i = 1}^n (\left\lceil\frac{s_i}{s_\text{chunk}}\right\rceil)

where

:math:`t_\text{word}` is the value specified by :kconfig:option:`CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US`.
:math:`t_\text{chunk}` is the value specified by :kconfig:option:`CONFIG_EMDS_CHUNK_PREPARATION_TIME_US`.
:math:`s_i` is the size of the :math:`i`\ th entry in bytes(entry data length + 4 bytes entry header).
:math:`s_\text{meta}` is the size of the snapshot metadata (32 bytes).
:math:`s_\text{block}` is the number of bytes in one word (4 bytes).
:math:`s_\text{chunk}` is the number of bytes in one chunk (16 bytes).

Example of time estimation
==========================

Using the formula from the previous section, you can estimate the time required to store all entries for the :ref:`bluetooth_mesh_light_lc` sample running on the nRF52840.
The following values can be inserted into the formula:

*  Set :math:`t_\text{chunk}` = 31 µs and :math:`t_\text{word}` = 41 µs.
   These values are valid only for this specific chip and configuration, and should be computed for the specific configuration whenever using EMDS.
*  The sample uses two entries, one for the RPL with 255 entries (:math:`s_i` = 2040 + 4 B) and one for the lightness state (:math:`s_i` = 3 + 4 B).
*  The flash write block size :math:`s_\text{block}` in this case is 4 B.
*  The chunk size :math:`s_\text{chunk}` is 16 B.
*  The size of the snapshot metadata :math:`s_\text{meta}` is 32 B.

This gives the following formula to compute estimated storage time:

.. math::
   \begin{aligned}
   t_\text{store} = 41{ µs}(\frac{32{ B}}{4{ B}}) + 41{ µs}(\frac{2044{ B} + 7{ B}}{4{ B}}) + 31{ µs}(\frac{2044{ B} + 7{ B}}{16{ B}}) = 25360\text{ µs}
   \end{aligned}

Calling the :c:func:`emds_store_time_get` function in the sample automatically computes the result of the formula and returns 25360.

Data storing context
====================

Despite that the :c:func:`emds_store` function blocks interrupts to prevent context pre-emption, it is recommended to call it from a context with the highest priority to minimize the chance of context pre-emption before the function starts executing.
The EMDS “data storing finished” callback is invoked from the same context as :c:func:`emds_store`.
If a radio is present, the :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>` and the :ref:`Multiprotocol Service Layer <mpsl_lib>` must be uninitialized before calling :c:func:`emds_store` to ensure that no radio activity is in progress.

Limitations
***********
    The power-fail comparator cannot be active when EMDS is used, as it will prevent the NVMC or RRAMC from performing write operations to persistent memory.

Dependencies
************
The emergency data storage is dependent on these Kconfig options:

* :kconfig:option:`CONFIG_FLASH_MAP`

API documentation
*****************

| Header file: :file:`include/emds/emds.h`
| Source file: :file:`subsys/emds/emds.c`

.. doxygengroup:: emds
