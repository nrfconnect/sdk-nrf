.. _partition_manager:

Partition Manager
#################

.. contents::
   :local:
   :depth: 2

The Partition Manager is a Python script that sets the start address and size of all the flash and RAM partitions in a multi-image build context.
When creating an application that requires child images, like a bootloader, you can configure the Partition Manager to control where each image should be placed in memory, and how the RAM should be shared.

See :ref:`ug_multi_image` for more information about multi-image builds.

The Partition Manager is activated for all multi-image builds, regardless of which build strategy is used for the child image.

.. _pm_overview:

Overview
********

The Partition Manager script reads the configuration files named :file:`pm.yml`, which define flash and RAM partitions.
A definition of a flash partition includes the name and the constraints on both size and placement in the flash.
A definition of a RAM partition includes the name and the constraints on its size.
The Partition Manager allocates a start address and, when set, a size to each partition in a way that complies with these constraints.

There are different types of **flash partitions** and **RAM partitions**, as described below.

Flash partition types
=====================

Image partitions
   An image partition is the flash area reserved for an image, to which the image binary is written.

   When the Partition Manager is active, there is one *root image* and one or more *child images*.
   The name of the root image is ``app``; it is always implicitly defined.
   Child images are explicitly defined in :file:`pm.yml` files.
   The size of the root image partition is dynamic, while the sizes of all child image partitions are statically defined.

Placeholder partitions
   A placeholder partition does not contain an image, but reserves space for other uses.
   For example, you might want to reserve space in flash to hold an updated application image while it is being downloaded and before it is copied to the application image partition.

Container partitions
   A container partition does not reserve space but is used to logically and/or physically group other partitions.

The start addresses and sizes of image partitions are used in the preprocessing of the linker script for each image.

RAM partition types
=====================

Default image RAM partition
   The default image RAM partition consists of all the RAM that is not defined as a permanent image RAM partition or placeholder RAM partition.
   It is the default RAM partition associated with an image and is set as the RAM region when linking the image.
   If an image must reserve its RAM area permanently (i.e. at the same time as other images are running), it must use a permanent image RAM partition, described below.

.. _pm_permanent_image_ram_partition:

Permanent image RAM partitions
   A permanent image RAM partition reserves RAM for an image permanently.
   It is typically used for images that will remain active after they have booted the next step in the boot chain.
   If an image has configured a permanent image RAM partition, it is set as the RAM region when linking the image, instead of the default image RAM partition.

.. _pm_permanent_placeholder_ram_partition:

Permanent placeholder RAM partitions
   A permanent placeholder RAM partition is used to permanently reserve RAM regions that are not associated with an image.

.. _pm_configuration:

Configuration
*************

Each child image must define its Partition Manager configuration in a file called :file:`pm.yml`.
This file must be stored in the same folder as the main :file:`CMakeLists.txt` file of the child image.

.. note::
   :file:`pm.yml` is only used for child images.
   The root application does not need to define a :file:`pm.yml` file, because its partition size and placement is implied by the size and placement of the child image partitions.
   If a root application defines a :file:`pm.yml` file, it is silently ignored.

The Partition Manager configuration can be also provided by a *subsystem*, intended as a software component.
For example, the support for a file system.
Subsystem Partition Manager configurations cannot define image partitions.

See :file:`subsys/partition_manager/CMakeLists.txt` for details on how to add the subsystem-specific Partition Manager configuration files.
The following code shows how the ``settings`` subsystem configuration is added.

.. code-block:: cmake

   if (CONFIG_SETTINGS_FCB OR CONFIG_SETTINGS_NVS)
     add_partition_manager_config(pm.yml.settings)
   endif()

When multiple application images, within the same domain, build the same subsystem code, there are some limitations if the code adds a Partition Manager configuration file using this methodology.
In particular, partition definitions are global per domain, and must be identical across all the calls to ``add_partition_manager_config()``.
If the same partition is defined twice with different configurations within a domain, the Partition Manager will fail.

.. note::
   If Partition Manager configurations are only defined by subsystems, so that only one image is included in the build, you must set the option :option:`CONFIG_PM_SINGLE_IMAGE` to execute the Partition Manager script.

.. _pm_yaml_format:

Configuration file format
=========================

A :file:`pm.yml` file contains partition definitions.
Each partition is defined as follows:

.. code-block:: yaml

   partition_name:
      partition_property:
         property_value

*partition_name* is the name of the partition (for example, ``mcuboot``).

The following partition properties and property values are available:

.. _partition_manager_placement:

placement: dict
   This property specifies the placement of the partition relative to other partitions, to the start or end of the flash, or to the root image ``app``.

   A partition with the placement property set is either an image partition or a placeholder partition.
   If the partition name is the same as the image name (as defined in a ``CMakeLists.txt``; see :ref:`ug_multi_image_defining` for details), this partition is the image partition.
   All other partitions are placeholder partitions.
   Each :file:`pm.yml` file must define exactly one image partition.

   The placement is formatted as a YAML dict.
   The valid keywords are listed below:

      before: list
         Place the partition before the first existing partition in the list.

      after: list
         Place the partition after the first existing partition in the list.

     Valid values in the lists are ``app``, ``start``, ``end``, or the name of any partition.
     The value ``start`` refers to the start address of the flash device's memory.
     The value ``end`` refers to its end address.
     It is not possible to place the partition after ``end`` or before ``start``.

      align: dict
         Ensure alignment of start or end of partition by specifying a dict with a ``start`` or ``end`` key respectively, where the value is the number of bytes to align to.
         If necessary, empty partitions are inserted in front of or behind the partition to ensure that the alignment is correct.
         Only one key can be specified.
         Partitions that directly or indirectly (through :ref:`spans <partition_manager_spans>`) share size with the ``app`` partitions can only be aligned if they are placed directly after the ``app`` partition.

.. _partition_manager_spans:

span: list OR span: string
   This property is used to define container partitions.
   Its value may be a list or string.

   Since this property is used to define container partitions, it cannot be used together with the ``placement`` property.

   If the value is a list, its elements are the names of the partitions that should be placed in the container:

The following example shows a partition that *spans*, or contains, ``partition_1`` through ``partition_n``, in any order:

   .. code-block:: yaml

      container_partition_name:
        span: [partition_1, partition_2, ..., partition_n]

   The list elements are interpreted as the set of potential partitions in the container, which the Partition Manager may place in the flash in any order.
   For example, ``partition_2`` could be placed before ``partition_1``.

   If the value is a string, it is interpreted as a list with one item:

The following 2 examples are equivalent:

   .. code-block:: yaml

      container_partition_name:
        span: foo

      container_partition_name:
        span: [foo]

   Non-existent partitions are removed from the ``span`` list before processing.
   Partitions with empty ``span`` lists are removed altogether, unless filled by the :ref:`inside property <partition_manager_inside>`.

   If the Partition Manager is forced to place a partition that is not declared in the ``span`` list between two partitions that are in the list, the configuration is unsatisfiable and therefore invalid.
   See :ref:`Span property example 1 <partition_manager_span_ex1>` for an example of an invalid configuration.

   .. note::
      You can specify configurations with an ambiguous ordering.
      Different versions of the Partition Manager script may produce different partition orders for such configurations, or fail to find a solution even if one is possible.
      The Partition Manager always detects unsatisfiable configurations (no false positives), but it might fail on some valid inputs (false negatives).

   Here are 3 examples of valid and invalid configurations:

   .. _partition_manager_span_ex1:

   * In the following example, the mcuboot and spm configurations result in this partition order: ``mcuboot``, ``spm``, ``app``.
     Therefore, the foo partition configuration is invalid, because ``spm`` must be placed between ``mcuboot`` and ``app``, but is not in the span list.

     .. code-block:: yaml
        :caption: Span property example 1 (invalid)

        mcuboot:
           placement:
              before: [spm, app]

        spm:
           placement:
              before: [app]

        foo:
           span: [mcuboot, app]

   * In the following example, these mcuboot, spm, and app configurations have two possible orders:

     * Order 1: mcuboot, spm, app
     * Order 2: mcuboot, app, spm

     In the absence of additional configuration, the Partition Manager may choose either order.
     However, since a span configuring the foo partition is present, the Partition Manager should choose order 2, since it is the only order that results in a valid configuration for the foo partition.

     .. code-block:: yaml
        :caption: Span property example 2 (valid)

        mcuboot:
           placement:

        spm:
           placement:
              after: [mcuboot]

        app:
           placement:
              after: [mcuboot]

        foo:
           span: [mcuboot, app]


   * In the following example, these mcuboot, spm, and app configurations have two possible orders:

     * Order 1: mcuboot, spm, app
     * Order 2: mcuboot, app, spm

     However, the overall configuration is unsatisfiable: foo requires order 2, while bar requires order 1.

     .. code-block:: yaml
        :caption: Span property example 3 (invalid)

        mcuboot:
           placement:

        spm:
           placement:
              after: [mcuboot]

        app:
           placement:
              after: [mcuboot]

        foo:
           span: [mcuboot, app]

        bar:
           span: [mcuboot, spm]

.. _partition_manager_inside:

inside: list
   This property is the inverse of ``span``.
   The name of the partition that specifies this property is added to the ``span`` list of the first existing container partition in the list.
   This property can be set for image or placeholder partitions.

   .. code-block:: yaml
      :caption: Example for the inside property

      mcuboot:
         inside: [b0]

      b0:
         span: [] # During processing, this span will contain mcuboot.

size: hexadecimal value
   This property defines the size of the partition.
   You can provide a Kconfig option as the value, which allows the user to easily modify the size (see :ref:`pm_yaml_preprocessing` for an example).

share_size: list
   This property defines the size of the current partition to be the same as the size of the first existing partition in the list.
   This property can be set for image or placeholder partitions.
   It cannot be used by container partitions.
   The list can contain any kind of partition.
   ``share_size`` takes precedence over ``size`` if one or more partitions in ``share_size`` exists.

   If the target partition is the ``app`` or a partition that spans over the ``app``, the size is effectively split between them, because the size of the ``app`` is dynamically decided.

   If none of the partitions in the ``share_size`` list exists, and the partition does not define a ``size`` property, then the partition is removed.
   If none of the partitions in the ``share_size`` list exists, and the partition *does* define a ``size`` property, then the ``size`` property is used to set the size.

region: string
   Specify the region where a partition should be placed.
   See :ref:`pm_regions`.

.. _partition_manager_ram_configuration:

RAM partition configuration
   RAM partitions are partitions located in the ``sram_primary`` region.
   A RAM partition is specified by having the partition name end with ``_sram``.
   If a partition name is composed of an image name plus the ``_sram`` ending, it is used as a permanent image RAM partition for the image.

The following 2 examples are equivalent:

   .. code-block:: yaml
      :caption: RAM partition configuration, without the ``_sram`` ending.

      some_permament_sram_block_used_for_logging:
         size: 0x1000
         region: sram_primary

   .. code-block:: yaml
      :caption: RAM partition configuration, using the ``_sram`` ending.

      some_permament_sram_block_used_for_logging_sram:
         size: 0x1000

The following example specifies a permanent image RAM partition for MCUboot, that will be used by the MCUboot linker script.

   .. code-block:: yaml

      mcuboot_sram:
          size: 0xa000

All occurrences of a partition name can be replaced by a dict with the key ``one_of``.
This dict is resolved to the first existing partition in the ``one_of`` value.
The value of the ``one_of`` key must be a list of placeholder or image partitions, and it cannot be a span.

See the following 2 examples, they are equivalent:

   .. code-block:: yaml
      :caption: Example use of a ``one_of`` dict

      some_span:
         span: [something, {one_of: [does_not_exist_0, does_not_exist_1, exists1, exists2]}]

   .. code-block:: yaml
      :caption: Example not using a ``one_of`` dict

      some_span:
         span: [something, exists1]

An error is triggered if none of the partitions listed inside the ``one_of`` dict exists.

To use this functionality, the properties that must explicitly define the ``one_of`` keyword are the following:

* ``span``
* ``share_size``

The :ref:`placement property <partition_manager_placement>` contains the functionality of ``one_of`` by default.
As such, you must not use ``one_of`` with the ``placement`` property.
Doing so will trigger a build error.

The keywords ``before`` and ``after`` already check for the first existing partition in their list.
Therefore, you can pass a list of partitions into these keywords.


.. _pm_yaml_preprocessing:

Configuration file preprocessing
================================

Each :file:`pm.yml` file is preprocessed to resolve symbols from Kconfig and devicetree.

The following example is taken from the :file:`pm.yml` file for the :ref:`immutable_bootloader` provided with the  |NCS|.
It includes :file:`autoconf.h` and :file:`devicetree_legacy_unfixed.h` (generated by Kconfig and devicetree respectively) to read application configurations and hardware properties.
In this example the application configuration is used to configure the size of the image and placeholder partitions.
The application configuration is also used to decide in which region the ``otp`` partition should be stored.
The information extracted from devicetree is the alignment value for some partitions.


.. code-block:: yaml

   #include <autoconf.h>
   #include <devicetree_legacy_unfixed.h>

   b0:
     size: CONFIG_PM_PARTITION_SIZE_B0_IMAGE
     placement:
       after: start

   b0_container:
     span: [b0, provision]

   s0_pad:
     share_size: mcuboot_pad
     placement:
       after: b0
       align: {start: CONFIG_FPROTECT_BLOCK_SIZE}

   spm_app:
     span: [spm, app]

   s0_image:
     # S0 spans over the image booted by B0
     span: {one_of: [mcuboot, spm_app]}

   s0:
     # Phony container to allow hex overriding
     span: [s0_pad, s0_image]

   s1_pad:
     # This partition will only exist if mcuboot_pad exists.
     share_size: mcuboot_pad
     placement:
       after: s0
       align: {start: DT_FLASH_ERASE_BLOCK_SIZE}

   s1_image:
     share_size: {one_of: [mcuboot, s0_image]}
     placement:
       after: [s1_pad, s0]
       align: {end: CONFIG_FPROTECT_BLOCK_SIZE}

   s1:
     # Partition which contains the whole S1 partition.
     span: [s1_pad, s1_image]

   provision:
     size: CONFIG_PM_PARTITION_SIZE_PROVISION
   #if defined(CONFIG_SOC_NRF9160) || defined(CONFIG_SOC_NRF5340_CPUAPP)
     region: otp
   #else
     placement:
       after: b0
       align: {start: DT_FLASH_ERASE_BLOCK_SIZE}
   #endif

.. _pm_regions:

Regions
=======

The Partition Manager places partitions in different *regions*.
For example, you can use regions for internal flash memory and external flash memory.

To define in which region a partition should be placed, use the ``region`` property in the configuration of the partition.
If no region is specified, the predefined internal flash region is used.

Defining a region
-----------------

Each region is defined by a name, a start address, a size, a placement strategy, and, if applicable, a device name.
A region only specifies a device name if there is a device driver associated with the region, for example, a driver for an external SPI flash.

There are three types of placement strategies, which affect how partitions are placed in regions:

start_to_end
   Place partitions sequentially from start to end.
   Partitions stored in a region with this placement strategy cannot affect their placement through the ``placement`` property.
   The unused part of the region is assigned to a partition with the same name as the region.

end_to_start
   Place partitions sequentially from end to start.
   Partitions stored in a region with this placement strategy cannot affect their placement through the ``placement`` property.
   The unused part of the region is exposed through a partition with the same name as the region.

complex
   Place partitions according to their ``placement`` configuration.
   The unused part of the region is exposed through a partition named ``app``.

Regions are defined in :file:`partition_manager.cmake`.
For example, see the following definitions for default regions:

.. code-block:: cmake

  add_region(     # Define region without device name
    otp           # Name
    756           # Size
    0xff8108      # Base address
    start_to_end  # Placement strategy
    )

  add_region_with_dev(           # Define region with device name
    flash_primary                # Name
    ${flash_size}                # Size
    ${CONFIG_FLASH_BASE_ADDRESS} # Base address
    complex                      # Placement strategy
    NRF_FLASH_DRV_NAME           # Device name
    )

.. _pm_external_flash:

External flash
==============

The Partition Manager supports partitions in the external flash memory through the use of :ref:`pm_regions`.
Any placeholder partition can specify that it should be stored in the external flash region.
External flash regions always use the start_to_end placement strategy.

To use external flash, you must provide information about the device to the Partition Manager through these Kconfig options:

* :option:`CONFIG_PM_EXTERNAL_FLASH` - enable external flash
* :option:`CONFIG_PM_EXTERNAL_FLASH_DEV_NAME` - specify the name of the flash device
* :option:`CONFIG_PM_EXTERNAL_FLASH_BASE` - specify the base address
* :option:`CONFIG_PM_EXTERNAL_FLASH_SIZE` - specify the available flash size (from the base address)

The following example assumes that the flash device has been initialized as follows in the flash driver:

.. code-block:: c

   DEVICE_AND_API_INIT(spi_flash_memory, "name_of_flash_device", ... );


To enable external flash support in the Partition Manager, configure the following options:

.. code-block:: Kconfig

   # prj.conf of application
   CONFIG_PM_EXTERNAL_FLASH=y
   CONFIG_PM_EXTERNAL_FLASH_DEV_NAME="name_of_flash_device"
   CONFIG_PM_EXTERNAL_FLASH_BASE=0x1000  # Don't touch magic stuff at the start
   CONFIG_PM_EXTERNAL_FLASH_SIZE=0x7F000 # Total size of external flash from base

Now partitions can be placed in the external flash:

.. code-block:: yaml

   # Name of partition
   external_plz:
     region: external_flash
     size: CONFIG_EXTERNAL_PLZ_SIZE

.. _pm_build_system:

Build system
************

The build system finds the child images that have been enabled and their configurations.

For each image, the Partition Manager's CMake code infers the paths to the following files and folders from the name and from other global properties:

* The :file:`pm.yml` file
* The compiled HEX file
* The generated include folder

After CMake finishes configuring the child images, the Partition Manager script is executed in configure time (``execute_process``) with the lists of names and paths as arguments.
The configurations generated by the Partition Manager script are imported as CMake variables (see :ref:`pm_cmake_usage`).

The Partition Manager script outputs a :file:`partitions.yml` file.
This file contains the internal state of the Partition Manager at the end of processing.
This means it contains the merged contents of all :file:`pm.yml` files, the sizes and addresses of all partitions, and other information generated by the Partition Manager.



.. _pm_generated_output_and_usage:

Generated output
================

After the main Partition Manager script has finished, another script runs.
This script takes the :file:`partitions.yml` file as input and creates the following output files:

* A C header file :file:`pm_config.h` for each child image and for the root application
* A key-value file :file:`pm.config`

The header files are used in the C code, while the key-value file is imported into the CMake namespace.
Both kinds of files contain, among other information, the start address and size of all partitions.

Usage
=====

The output that the Partition Manager generates can be used in various areas of your code.

C code
------

When the Partition Manager is enabled, all source files are compiled with the define ``USE_PARTITION_MANAGER`` set to 1.
If you use this define in your code, the preprocessor can choose what code to include depending on whether the Partition Manager is being used.

.. code-block:: C

   #if USE_PARTITION_MANAGER
   #include <pm_config.h>
   #define NON_SECURE_APP_ADDRESS PM_APP_ADDRESS
   #else
   ...

HEX files
---------

The Partition Manager may implicitly or explicitly assign a HEX file to a partition.

Image partitions are implicitly assigned the compiled HEX file, i.e. the HEX file that is generated when building the corresponding image.
Container partitions are implicitly assigned the result of merging the HEX files that are assigned to the underlying partitions.
Placeholder partitions are not implicitly assigned a HEX file.

To explicitly assign a HEX file to a partition, set the global properties *partition_name*\ _PM_HEX_FILE and *partition_name*\ _PM_TARGET in CMake, where *partition_name* is the name of the partition.
*partition_name*\ _PM_TARGET specifies the build target that generates the HEX file specified in *partition_name*\ _PM_HEX_FILE.

See the following example, which assigns a cryptographically signed HEX file built by the ``sign_target`` build target to the root application:


.. code-block:: cmake

   set_property(
     GLOBAL PROPERTY
     app_PM_HEX_FILE # Must match "*_PM_HEX_FILE"
     ${PROJECT_BINARY_DIR}/signed.hex
   )

   set_property(
     GLOBAL PROPERTY
     app_PM_TARGET # Must match "*_PM_TARGET"
     sign_target
   )


As output, the Partition Manager creates a HEX file called :file:`merged.hex`, which is programmed to the development kit when calling ``ninja flash``.
When creating :file:`merged.hex`, all assigned HEX files are included in the merge operation.
If the HEX files overlap, the conflict is resolved as follows:

* HEX files assigned to container partitions overwrite HEX files assigned to their underlying partitions.
* HEX files assigned to larger partitions overwrite HEX files assigned to smaller partitions.
* Explicitly assigned HEX files overwrite implicitly assigned HEX files.

This means that you can overwrite a partition's HEX file by wrapping that partition in another partition and assigning a HEX file to the new partition.

ROM report
----------

When using the Partition Manager, run ``ninja rom_report`` to see the addresses and sizes of flash partitions.

.. _pm_cmake_usage:

CMake
-----

The CMake variables from the Partition Manager are typically used through `generator expressions`_, because these variables are only made available late in the CMake configure stage.
To read a Partition Manager variable through a generator expression, the variable must be assigned as a target property.
The Partition Manager stores all variables as target properties on the ``partition_manager`` target, which means they can be used in generator expressions in the following way:

.. code-block:: none
   :caption: Reading Partition Manager variables in generator expressions

   --slot-size $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PARTITIONS_PRIMARY_SIZE>

.. _ug_pm_static:

Static configuration
********************

By default, the Partition Manager dynamically places the partitions in memory.
However, if you have a deployed product that consists of multiple images, where only a subset of the included images can be upgraded through a firmware update mechanism, the upgradable images must be statically configured.
For example, if a device includes a non-upgradable first-stage bootloader and an upgradable application, the application image to be upgraded must be linked to the same address as the one that is deployed.

For this purpose, the Partition Manager provides static configuration to define static partitions.
The area used by the static partitions is called the *static area*.
The static area comes in addition to the *dynamic area*, which consists of the ``app`` partition and all memory adjacent to the ``app`` partition that is not occupied by a static partition.
Note that there is only one dynamic area.
When the Partition Manager is executed, it operates only on the dynamic area, assuming that all other memory is reserved.

Within the dynamic area, you can define new partitions or configure existing partitions even if you are using static partitions.
The dynamic area is resized as required when updating the static configuration.

.. _ug_pm_static_providing:

Configuring static partitions
=============================

Static partitions are defined through a YAML-formatted configuration file in the root application's source directory.
This file is similar to the regular :file:`pm.yml` configuration files, except that it also defines the start address for all partitions.

The static configuration can be provided through a :file:`pm_static.yml` file in the application's source directory.
Alternatively, define a ``PM_STATIC_YML_FILE`` variable that provides the path and file name for the static configuration in the application's :file:`CMakeLists.txt` file, as shown in the example below.

.. code-block:: cmake

   set(PM_STATIC_YML_FILE
     ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_${CMAKE_BUILD_TYPE}.yml
     )

Use a static partition layout to ensure consistency between builds, as the settings storage will be at the same location after the DFU.

The current partition configuration for a build can be found in :file:`${BUILD_DIR}/partitions.yml`.
To apply the current configuration as a static configuration, copy this file to :file:`${APPLICATION_SOURCE_DIR}/pm_static.yml`.

It is also possible to build a :file:`pm_static.yml` from scratch by following the description in :ref:`ug_pm_static_add`

When modifying static configurations, keep in mind the following:

* There can only be one unoccupied gap per region.
* All statically defined partitions in regions with ``end_to_start`` or ``start_to_end`` placement strategy must be packed at the end or the start of the region, respectively.

The default ``flash_primary`` region uses the ``complex`` placement strategy, so these limitations do not apply there.

You can add or remove partitions as described in the following sections.

.. note::
  If the static configuration contains an entry for the ``app`` partition, this entry is ignored.

.. _ug_pm_static_add_dynamic:

Adding a dynamic partition
--------------------------

New dynamic partitions that are listed in a :file:`pm.yml` file are automatically added.
However, if a partition is defined both as a static partition and as a dynamic partition, the dynamic definition is ignored.

.. note::
   When resolving the relative placement of dynamic partitions, any placement properties referencing static partitions are ignored.

.. _ug_pm_static_add:

Adding a static partition
-------------------------

To add a static partition, add an entry for it in :file:`pm_static.yml`.
This entry must define the properties ``address``, ``size``, and - if applicable - ``span``.
The region defaults to ``flash_primary`` if no ``region`` property is specified.

.. code-block:: yaml
   :caption: Example of static configuration of a partition with span

   partition_name:
      address: 0xab00
      size: 0x1000
      span: [example]  # Only if this partition had the span property set originally.

.. note::
  Child images that are built with the build strategy *partition_name*\ _BUILD_STRATEGY_SKIP_BUILD or *partition_name*\ _BUILD_STRATEGY_USE_HEX_FILE must define a static partition to ensure correct placement of the dynamic partitions.

.. _ug_pm_static_remove:

Removing a static partition
---------------------------

To remove a static partition, delete its entry in :file:`pm_static.yml`.

Only partitions adjacent to the ``app`` partition or other removed partitions can be removed.
