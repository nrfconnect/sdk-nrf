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

.. note::
   When you build a multi-image application using the Partition Manager, the devicetree source flash partitions are ignored.

.. _pm_overview:

Overview
********

The Partition Manager script reads the configuration files named :file:`pm.yml`, which define flash and RAM partitions.
A definition of a flash partition includes the name and the constraints on both size and placement in the flash memory.
A definition of a RAM partition includes the name and the constraints on its size.
The Partition Manager allocates a start address and, when set, a size to each partition in a way that complies with these constraints.

There are different types of *flash partitions* and *RAM partitions*, as described in the following section.

Flash partition types
=====================

Image partitions
   An image partition is the flash memory area reserved for an image to which the image binary is written.

   When the Partition Manager is active, there is one *root image* and one or more *child images*.
   The name of the root image is ``app``.
   It is always implicitly defined.
   Child images are explicitly defined in :file:`pm.yml` files.
   The size of the root image partition is dynamic, while the sizes of all child image partitions are statically defined.

Placeholder partitions
   A placeholder partition does not contain an image, but reserves space for other uses.
   For example, you might want to reserve space in the flash memory to hold an updated application image while it is being downloaded and before it is copied to the application image partition.

Container partitions
   A container partition does not reserve space but is used to logically and (or) physically group other partitions.

The start addresses and sizes of image partitions are used in the preprocessing of the linker script for each image.

RAM partition types
===================

Default image RAM partition
   The default image RAM partition consists of all the RAM that is not defined as a permanent image RAM partition or placeholder RAM partition.
   It is the default RAM partition associated with an image and is set as the RAM region when linking the image.
   If an image must reserve its RAM area permanently (that is, at the same time as other images are running), it must use a permanent image RAM partition, described below.

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
     ncs_add_partition_manager_config(pm.yml.settings)
   endif()

When multiple application images, within the same domain, build the same subsystem code, there are some limitations if the code adds a Partition Manager configuration file using this methodology.
In particular, partition definitions are global per domain, and must be identical across all the calls to ``ncs_add_partition_manager_config()``.
If the same partition is defined twice with different configurations within a domain, the Partition Manager will fail.

.. note::
   If Partition Manager configurations are only defined by subsystems, so that only one image is included in the build, you must set the option :kconfig:option:`CONFIG_PM_SINGLE_IMAGE` to execute the Partition Manager script.

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
         Ensure the alignment of the start or the end of the partition by specifying a dict with a ``start`` or ``end`` key respectively, where the value is the number of bytes to align to.
         If necessary, empty partitions are inserted in front of or behind the partition to ensure that the alignment is correct.
         Only one key can be specified.
         Partitions that directly or indirectly (through :ref:`spans <partition_manager_spans>`) share size with the ``app`` partitions can only be aligned if they are placed directly after the ``app`` partition.

      align_next: int
         Ensure that the _next_ partition is aligned on this number of bytes.
         This is equivalent to ensuring the alignment of the start of the next partition.
         If the start of the next partition is already aligned, the largest alignment takes effect.
         ``align_next`` fails if the alignment of the start of the next partition is not a divisor or multiple of the ``align_next`` value.
         ``align_next`` also fails if the end of the next partition is aligned.

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

The following two examples are equivalent:

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

   Here are three examples of valid and invalid configurations:

   .. _partition_manager_span_ex1:

   * In the following example, the mcuboot and tfm configurations result in this partition order: ``mcuboot``, ``tfm``, ``app``.
     Therefore, the foo partition configuration is invalid, because ``tfm`` must be placed between ``mcuboot`` and ``app``, but is not in the span list.

     .. code-block:: yaml
        :caption: Span property example 1 (invalid)

        mcuboot:
           placement:
              before: [tfm, app]

        tfm:
           placement:
              before: [app]

        foo:
           span: [mcuboot, app]

   * In the following example, these mcuboot, tfm, and app configurations have two possible orders:

     * Order 1: mcuboot, tfm, app
     * Order 2: mcuboot, app, tfm

     In the absence of additional configuration, the Partition Manager may choose either order.
     However, since a span configuring the foo partition is present, the Partition Manager should choose order 2, since it is the only order that results in a valid configuration for the foo partition.

     .. code-block:: yaml
        :caption: Span property example 2 (valid)

        mcuboot:
           placement:

        tfm:
           placement:
              after: [mcuboot]

        app:
           placement:
              after: [mcuboot]

        foo:
           span: [mcuboot, app]


   * In the following example, these mcuboot, tfm, and app configurations have two possible orders:

     * Order 1: mcuboot, tfm, app
     * Order 2: mcuboot, app, tfm

     However, the overall configuration is unsatisfiable: foo requires order 2, while bar requires order 1.

     .. code-block:: yaml
        :caption: Span property example 3 (invalid)

        mcuboot:
           placement:

        tfm:
           placement:
              after: [mcuboot]

        app:
           placement:
              after: [mcuboot]

        foo:
           span: [mcuboot, app]

        bar:
           span: [mcuboot, tfm]

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

affiliation: string or list
   This property groups the partition with other partitions with the specified affiliation.
   Affiliations are used to generate ``PM_FOREACH_AFFILIATED_TO_<affliation>(fn)`` macros, which let you invoke the macro ``fn`` on all definitions of the partitions with the specified affiliation.
   The macro passes the partition name from the YAML file to the ``fn`` macro, using the upper-case formatting.
   Currently, the ``disk`` affiliation is reserved by the flash disk driver in Zephyr to generate disk objects from partitions defined by the Partition Manager.

extra_params: dict
   This is a dictionary of extra ``<param_name>`` parameters for the partition.
   The Partition Manager only uses them for generating ``PM_<uppercase_partition_name>_EXTRA_PARAM_<param_name>`` definitions, with the value taken from the YAML file, as assigned to the extra parameter.
   The Partition Manager does not use or process these parameters in any other way.
   Extra parameters can be used by other subsystems to add additional information to partitions.
   Currently, this feature is only used by the flash disk driver in Zephyr to generate disk objects for use with the Disk Access API, with the following extra parameters: ``disk_sector_size``, ``disk_cache_size``, ``disk_name``, and ``disk_read_only``.
   The following code snippet shows an example of the disk partition configuration using the ``extra_params`` dictionary:

   .. code-block:: yaml
      :caption: Example of disk partition

      fatfs_storage:
          affiliation: disk
          extra_params: {
              disk_name: "SD",
              disk_cache_size: 4096,
              disk_sector_size: 512,
              disk_read_only: 0
          }
          placement:
              before: [end]
              align: {start: 4096}
          inside: [nonsecure_storage]
          size: 65536

.. _partition_manager_ram_configuration:

RAM partition configuration
   RAM partitions are partitions located in the ``sram_primary`` region.
   A RAM partition is specified by having the partition name end with ``_sram``.
   If a partition name is composed of an image name plus the ``_sram`` ending, it is used as a permanent image RAM partition for the image.

The following two examples are equivalent:

   .. code-block:: yaml
      :caption: RAM partition configuration, without the ``_sram`` ending

      some_permament_sram_block_used_for_logging:
         size: 0x1000
         region: sram_primary

   .. code-block:: yaml
      :caption: RAM partition configuration, using the ``_sram`` ending

      some_permament_sram_block_used_for_logging_sram:
         size: 0x1000

The following example specifies a permanent image RAM partition for MCUboot, that will be used by the MCUboot linker script.

   .. code-block:: yaml

      mcuboot_sram:
          size: 0xa000

All occurrences of a partition name can be replaced by a dict with the key ``one_of``.
This dict is resolved to the first existing partition in the ``one_of`` value.
The value of the ``one_of`` key must be a list of placeholder or image partitions, and it cannot be a span.

See the following two examples, they are equivalent:

   .. code-block:: yaml
      :caption: Example of using a ``one_of`` dict

      some_span:
         span: [something, {one_of: [does_not_exist_0, does_not_exist_1, exists1, exists2]}]

   .. code-block:: yaml
      :caption: Example of not using a ``one_of`` dict

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

The following example is taken from the :file:`pm.yml` file for the :ref:`immutable_bootloader` provided with the |NCS|.
It includes :file:`autoconf.h` and :file:`devicetree_legacy_unfixed.h` (generated by Kconfig and devicetree respectively) to read application configurations and hardware properties.
In this example the application configuration is used to configure the size of the image and placeholder partitions.
The application configuration is also used to decide in which region the ``otp`` partition should be stored.
The information extracted from devicetree is the alignment value for some partitions.


.. code-block:: yaml

   #include <zephyr/autoconf.h>
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

   app_image:
     span: [tfm, app]

   s0_image:
     # S0 spans over the image booted by B0
     span: {one_of: [mcuboot, app_image]}

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
   #if defined(CONFIG_SOC_SERIES_NRF91X) || defined(CONFIG_SOC_NRF5340_CPUAPP)
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

  add_region(                           # Define region without device name
    NAME otp                            # Name
    SIZE 756                            # Size
    BASE 0xff8108                       # Base address
    PLACEMENT start_to_end              # Placement strategy
    )

  add_region(                           # Define region with device name
    NAME flash_primary                  # Name
    SIZE ${flash_size}                  # Size
    BASE ${CONFIG_FLASH_BASE_ADDRESS}   # Base address
    PLACEMENT complex                   # Placement strategy
    DEVICE flash_controller             # DTS node label of flash controller
    DEFAULT_DRIVER_KCONFIG	        # Kconfig option that should be set for
                                        # the driver to be compiled in
    )

.. _pm_external_flash:

External flash memory partitions
================================

The Partition Manager supports partitions in the external flash memory through the use of :ref:`pm_regions`.
Any placeholder partition can specify that it should be stored in the external flash memory region.
External flash memory regions always use the *start_to_end* placement strategy.

To store partitions in the external flash memory, you can either choose a value for the ``nordic,pm-ext-flash`` property in the devicetree, or directly use an external flash DTS node label as ``DEVICE``.
See the following example of an overlay file that sets this value:

.. literalinclude:: ../../tests/modules/mcuboot/external_flash/boards/nrf52840dk_nrf52840.overlay
    :language: c
    :caption: nrf52840dk_nrf52840.overlay

After the ``nordic,pm-ext-flash`` value is set, you can place partitions in the external flash memory as follows:

.. code-block:: yaml

   # Name of partition
   external_plz:
     region: external_flash
     size: CONFIG_EXTERNAL_PLZ_SIZE

.. note::

   * Use the :kconfig:option:`CONFIG_PM_PARTITION_REGION_LITTLEFS_EXTERNAL`, :kconfig:option:`CONFIG_PM_PARTITION_REGION_SETTINGS_STORAGE_EXTERNAL`, and :kconfig:option:`CONFIG_PM_PARTITION_REGION_NVS_STORAGE_EXTERNAL` to specify that the relevant partition must be located in external flash memory.
     You must add a ``chosen`` entry for ``nordic,pm-ext-flash`` in your devicetree to make this option available.
     See :file:`tests/subsys/partition_manager` for example configurations.

   * If the external flash device is not using the :dtcompatible:`nordic,qspi-nor` driver, you must enable :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK` to override the Partition Manager's external flash driver check, and the required driver must also be enabled for all applications that need it.

See :ref:`ug_bootloader_external_flash` for more details on using external flash memory with MCUboot.

.. _pm_build_system:

A partition can be accessible at runtime only if the flash device where it resides has its driver enabled at compile time.
Partition manager ignores partitions that are located in a region without its driver enabled.
To let partition manager know which Kconfig option ensures the existence of the driver, the option ``DEFAULT_DRIVER_KCONFIG`` is used.

For partitions in the internal flash memory, ``DEFAULT_DRIVER_KCONFIG`` within :file:`partition_manager.cmake` is set automatically to :kconfig:option:`CONFIG_SOC_FLASH_NRF`, so that all the partitions placed in the internal flash are always available at runtime.
For external regions, ``DEFAULT_DRIVER_KCONFIG`` within :file:`partition_manager.cmake` must be set to :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_HAS_DRIVER`.
Out-of-tree drivers can select this value to attest that they provide support for the external flash.
This is a hidden option and can be selected only by an external driver or a Kconfig option.

This option is automatically set when :kconfig:option:`CONFIG_NRF_QSPI_NOR` or :kconfig:option:`CONFIG_SPI_NOR` is enabled.
If the application provides the driver in an unusual way, this option can be overridden by setting :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK` in the application configuration.

As partition manager does not know if partitions are used at runtime, consider the following:

  * Enabling Kconfig options that affect ``DEFAULT_DRIVER_KCONFIG`` will add a partition map entry for the partition depending on it, whether it is used at runtime or not.
  * Not enabling the Kconfig options that affect ``DEFAULT_DRIVER_KCONFIG`` will not add partition map entry for partition depending on it, whether it is used at runtime or not.
  * Enabling Kconfig options that affect ``DEFAULT_DRIVER_KCONFIG`` can cause linker errors when the option has no effect on including a driver into compilation.
    In this case, partition manager adds a partition map entry that has a pointer to the flash device it is supposed to be placed on, but due to misconfiguration the driver is actually not compiled in.
    This situation can also be caused by setting the :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK`, as partition manager will just assume that the driver is provided by the application.

.. note::

   When using an application configured with an MCUboot child image, both images use the same partition manager configuration, which means that the app and MCUboot have exactly the same partition maps.
   The accessibility at runtime of flash partitions depends on the configurations of both the application and MCUboot and the values they give to the ``DEFAULT_DRIVER_KCONFIG`` option of the partition manager region specification.

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

Image partitions are implicitly assigned the compiled HEX file, that is, the HEX file that is generated when building the corresponding image.
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

.. _pm_generated_output_and_usage_pm_report:

Partition Manager report
------------------------

When using the Partition Manager, run ``ninja partition_manager_report`` to see the addresses and sizes of all the configured partitions.

.. _pm_cmake_usage:

CMake
-----

The CMake variables from the Partition Manager are typically used through `generator expressions`_, because these variables are only made available late in the CMake configure stage.
To read a Partition Manager variable through a generator expression, the variable must be assigned as a target property.
The Partition Manager stores all variables as target properties on the ``partition_manager`` target, which means they can be used in generator expressions in the following way:

.. code-block:: none
   :caption: Reading Partition Manager variables in generator expressions

   --slot-size $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PARTITIONS_PRIMARY_SIZE>

.. _pm_partition_reports:

Partition placement reports
---------------------------

You can generate an ASCII-art map report to get an overview of how the partition manager creates flash memory partitions.
This is especially useful when using multiple bootloaders.

To generate the report, use the ``ninja partition_manager_report`` or ``west build -t partition_manager_report`` commands.

For example, if you generate a partition placement report on the build of :file:`zephyr/samples/hello_world` with the nRF52840 development kit using |NSIB| and MCUboot, you would get as an output an ASCII partition map that looks like the following:

.. code-block:: console

    (0x100000 - 1024.0kB):
   +------------------------------------------+
   +---0x0: b0 (0x9000)-----------------------+
   | 0x0: b0_image (0x8000)                   |
   | 0x8000: provision (0x1000)               |
   +---0x9000: s0 (0xc200)--------------------+
   | 0x9000: s0_pad (0x200)                   |
   +---0x9200: s0_image (0xc000)--------------+
   | 0x9200: mcuboot (0xc000)                 |
   | 0x15200: EMPTY_0 (0xe00)                 |
   +---0x16000: s1 (0xc200)-------------------+
   | 0x16000: s1_pad (0x200)                  |
   | 0x16200: EMPTY_1 (0xe00)                 |
   | 0x17000: s1_image (0xc000)               |
   +---0x23000: mcuboot_primary (0x6e000)-----+
   | 0x23000: mcuboot_pad (0x200)             |
   +---0x23200: mcuboot_primary_app (0x6de00)-+
   +---0x23200: app_image (0x6de00)-----------+
   | 0x23200: app (0x6de00)                   |
   | 0x91000: mcuboot_secondary (0x6e000)     |
   | 0xff000: EMPTY_2 (0x1000)                |
   +------------------------------------------+

The sizes of each partition are determined by the associated :file:`pm.yml` file, such as :file:`nrf/samples/bootloader/pm.yml` for |NSIB| and :file:`bootloader/mcuboot/boot/zephyr/pm.yml` for MCUboot.

.. _ug_pm_static:

Static and dynamic configuration
********************************

By default, the Partition Manager dynamically places the partitions in memory.
However, there are cases where a deployed product can consist of multiple images, and only a subset of these images can be upgraded through a firmware update mechanism.
In such cases, the upgradable images must have partitions that are static and are matching the partition map used by the bootloader programmed onto the device.
For example, if a device includes a non-upgradable first-stage bootloader and an upgradable application, the application image to be upgraded must be linked to the same address as the one that is deployed.
For this purpose, the Partition Manager provides the static configuration to define static partitions.

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

You can set :makevar:`PM_STATIC_YML_FILE` to contain exactly the static configuration you want to use.

If you do not set :makevar:`PM_STATIC_YML_FILE`, the build system will use the following order to look for files in your application source directory to use as a static configuration layout:

* If a :ref:`file suffix <app_build_file_suffixes>` is used, the following order applies:

  1. If the file :file:`pm_static_<board>_<revision>_<suffix>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static_<board>_<revision>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static_<board>_<suffix>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static_<board>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static_<suffix>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static.yml` exists, it will be used.

* If a suffixed configuration is not used, then the same order as above applies, except that *<suffix>* is not part of the file name:

  1. If the file :file:`pm_static_<board>_<revision>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static_<board>.yml` exists, it will be used.
  #. Otherwise, if the file :file:`pm_static.yml` exists, it will be used.

For :ref:`ug_multi_image` where the image targets a different domain, :ref:`ug_multi_image_build_scripts` uses the same search algorithm, but a domain specific configuration file is also searched.
For example, :file:`pm_static_<board>_<suffix>_<domain>.yml` or :file:`pm_static_<board>_<domain>.yml`.

Use a static partition layout to ensure consistency between builds, as the settings storage will be at the same location after the DFU.

The current partition configuration for a build can be found in :file:`${BUILD_DIR}/partitions.yml`.
To apply the current configuration as a static configuration, copy this file to :file:`${APPLICATION_CONFIG_DIR}/pm_static.yml`.

It is also possible to build a :file:`pm_static.yml` from scratch by following the description in :ref:`ug_pm_static_add`

When modifying static configurations, keep in mind the following:

* There can only be one unoccupied gap per region.
* All statically defined partitions in regions with ``end_to_start`` or ``start_to_end`` placement strategy must be packed at the end or the start of the region, respectively.

The default ``flash_primary`` region uses the ``complex`` placement strategy, so these limitations do not apply there.

You can add or remove partitions as described in the following sections.

.. note::
  If the static configuration contains an entry for the ``app`` partition, this entry is ignored.

.. _ug_pm_config_dynamic:

Configuring dynamic partitions
==============================

For child images that use dynamic partition sizing, the Partition Manager allows you to use the ``CONFIG_PM_PARTITION_SIZE_<CHILD_IMAGE>`` Kconfig option to adjust the partition size.
This is particularly useful when you want to optimize the memory usage of MCUboot to the smallest possible size by enabling cryptographic functionalities.

.. note::
   To match the reduced size, you need to set the ``CONFIG_PM_PARTITION_SIZE_<CHILD_IMAGE>`` Kconfig option in the child image's configuration.
   For example, ``CONFIG_PM_PARTITION_SIZE_MCUBOOT`` must be set for the MCUboot child image.

The ``CONFIG_PM_PARTITION_SIZE_<CHILD_IMAGE>`` Kconfig option allows you to specify the memory size for each child image's partition directly in the Kconfig file.
This allows for precise control over memory allocation which is crucial for system performance optimization.

Common variants of this Kconfig's option include the following:

* ``CONFIG_PM_PARTITION_SIZE_MCUBOOT`` - Sets the partition size for the MCUboot image, and only used for dynamic partition maps.
  You can read more about this option in the `MCUboot Kconfig option documentation`_.

* :kconfig:option:`CONFIG_PM_PARTITION_SIZE_PROVISION` - Allocates space for the provision partition used in secure boot scenarios.

* :kconfig:option:`CONFIG_PM_PARTITION_SIZE_SETTINGS` - Defines the size of the settings storage partition, typically used for persistent configuration data.

* :kconfig:option:`CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE` - Sets the size of the settings storage partition.

* :kconfig:option:`CONFIG_PM_PARTITION_SIZE_NVS_STORAGE` - Configures the size of the non-volatile storage partition.

You must adjust each variant according to the specific requirements and memory constraints of the project, ensuring optimal usage of available flash memory.
For a full list of the ``CONFIG_PM_PARTITION_SIZE`` variants, see the related `Kconfig search results`_.

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
