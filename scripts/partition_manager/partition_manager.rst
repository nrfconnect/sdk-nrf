.. _partition_manager:

Partition Manager
#################

The Partition Manager is a Python script that sets the start address and size of all flash and RAM partitions in a multi-image build context.
When creating an application that requires child images (for example, a bootloader), you can configure the Partition Manager to control where in memory each image should be placed, and how the RAM should be shared.

See :ref:`ug_multi_image` for more information about multi-image builds.

The Partition Manager is activated for all multi-image builds, no matter what build strategy is used for the child image.

.. _pm_overview:

Overview
********

The Partition Manager script reads configuration files named :file:`pm.yml`, which define flash and RAM partitions.
A flash partition's definition includes its name and constraints on its size and placement on flash.
A RAM partition's definition includes its name and constraints on its size.
The Partition Manager allocates a start address and sometimes a size to each partition in a way that satisfies these constraints.

There are four different kinds of **flash partitions**, and three different kinds of
**ram partitions**, as described below.

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
   A container partition does not reserve space, but is used to logically and/or physically group other partitions.

The start addresses and sizes of image partitions are used in the preprocessing of the linker script for each image.

RAM partition types
=====================

Image RAM partition (IRP)
   An IRP is the default RAM partition associated with an image.
   An IRP uses all RAM which is not defined as permanent image RAM partitions, or placeholder RAM partitions.
   An images IRP is only reserved while the image is running.
   Hence, when an image boots the next in the bootloader chain, its IRP is no longer reserved, and will be used as the IRP for the next image.
   All images has an IRP assigned to it by default.
   The start address and size of an IRP is used in the preprocessing of the linker script for its corresponding image.
   The alternative to IRP is Permanent Image RAM partitions (PIRP) described below.
   Note that an image can only have one type of RAM partition (IRP or PIRP).

Permanent Image RAM Partition (PIRP)
   A PIRP reserves RAM for an image permanently.
   This is typically used for images which will "live" after they have booted the next step in the boot chain.
   The start address and size of a PIRP is used in the preprocessing of the linker script for its corresponding image.

Permanent Placeholder RAM Partitions
   Permanent Placeholder RAM Partitions are used for reserving RAM regions permanently which are not associated with any images.


.. _pm_configuration:

Configuration
*************
Each child image must define its Partition Manager configuration in a file called :file:`pm.yml`.
This file must be stored in the same folder as the main :file:`CMakeLists.txt` file of the child image.

.. note::
   :file:`pm.yml` is only used for child images.
   The root application does not need to define a :file:`pm.yml` file, because its partition size and placement is implied by the size and placement of the child image partitions.
   If a root application defines a :file:`pm.yml` file, it is silently ignored.

The Partition Manager configuration can be also provided by a subsystem.
(Here, *subsystem* means a software component, like support for a file system.)
Subsystem Partition Manager configurations cannot define image partitions.

To add a Partition Manager configuration file for a subsystem, place the following line in its :file:`CMakeLists.txt` file (this is only required for subsystems, not applications):

.. code-block:: cmake

  add_partition_manager_config(pm.yml)

There are some limitations when multiple application images build the same subsystem code if it adds a Partition Manager configuration file in this way.
In particular, partition definitions are global and must be identical across calls to ``add_partition_manager_config()``.
If the same partition is defined twice with different configurations, the Partition Manager will fail.

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

placement: dict
   This property specifies the placement of the partition relative to other partitions, to the start or end of flash, or to the root image ``app``.

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
         Partitions which directly or indirectly (through :ref:`spans <partition_manager_spans>`) share size with the ``app`` partitions can only be aligned if they are placed directly after the ``app`` partition.

.. _partition_manager_spans:

span: list OR span: string

   This property is used to define container partitions.
   Its value may be a list or string.

   Since this property is used to define container partitions, it cannot be used together with the ``placement`` property.

   If the value is a list, its elements are the names of the partitions that should be placed in the container:

   .. code-block:: yaml

      # This partition spans, or contains, partition_1 through partition_n,
      # in any order:
      container_partition_name:
        span: [partition_1, partition_2, ..., partition_n]

   The list elements are interpreted as the set of potential partitions in the container, which the Partition Manager may place in flash in any order.
   For example, ``partition_2`` could be placed before ``partition_1``.

   If the value is a string, it is interpreted as a list with one item:

   .. code-block:: yaml

      # The following are equivalent:

      container_partition_name:
        span: foo

      container_partition_name:
        span: [foo]

   Non-existent partitions are removed from the ``span`` list before processing, and partitions with empty ``span`` lists are removed altogether (unless filled by the :ref:`inside property <partition_manager_inside>`).

   If the Partition Manager is forced to place a partition that is not declared in the ``span`` list between two partitions that are in the list, the configuration is unsatisfiable and therefore invalid.
   See :ref:`Span property example 1 <partition_manager_span_ex1>` for an example of an invalid configuration.

   .. note::
      You can specify configurations with an ambiguous ordering (see the following examples).
      Different versions of the Partition Manager script may produce different partition orders for such configurations, or fail to find a solution even if one is possible.
      The Partition Manager always detects unsatisfiable configurations (no false positives), but it might fail on some valid inputs (false negatives).

   Here are some examples of valid and invalid configurations.

   .. _partition_manager_span_ex1:

   .. code-block:: yaml
      :caption: Span property example 1 (invalid)

      # The mcuboot and spm configurations result in this partition order:
      # mcuboot, spm, app

      mcuboot:
         placement:
            before: [spm, app]

      spm:
         placement:
            before: [app]

      # Therefore, the foo partition configuration is invalid, because spm
      # must be placed between mcuboot and app, but is not in the span list:

      foo:
         span: [mcuboot, app]

   .. code-block:: yaml
      :caption: Span property example 2 (valid)

      # These mcuboot, spm, and app configurations have two possible orders:
      # Order 1: mcuboot, spm, app
      # Order 2: mcuboot, app, spm
      #
      # In the absence of additional configuration, the Partition Manager may
      # choose either order.

      mcuboot:
         placement:

      spm:
         placement:
            after: [mcuboot]

      app:
         placement:
            after: [mcuboot]

      # However, since the following span exists, the Partition Manager should
      # choose order 2, since it's the only order that results in a valid
      # configuration for the foo partition:

      foo:
         span: [mcuboot, app]


   .. code-block:: yaml
      :caption: Span property example 3 (invalid)

      # These mcuboot, spm, and app configurations have two possible orders:
      # Order 1: mcuboot, spm, app
      # Order 2: mcuboot, app, spm

      mcuboot:
         placement:

      spm:
         placement:
            after: [mcuboot]

      app:
         placement:
            after: [mcuboot]

      # However, the overall configuration is unsatisfiable:
      # foo requires order 2, while bar requires order 1.

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
   You can provide a Kconfig option as value, which allows the user to easily modify the size (see :ref:`pm_yaml_preprocessing` for an example).

share_size: list
   This properties defines the size of the current partition to be the same as the size of the first existing partition in the list.
   This property can be set for image or placeholder partitions.
   It cannot be used by container partitions.
   The list can contain any kind of partition.
   ``share_size`` takes precedence over ``size`` if one or more partitions in ``share_size`` exists.

   If the target partition is the ``app`` or a partition that spans over the ``app``, the size is effectively split between them, because the size of the ``app`` is dynamically decided.

   If none of the partitions in the ``share_size`` list exists, and the partition does not define a ``size`` property, then the partition is removed.
   If none of the partitions in the ``share_size`` list exists, and the partition **does** define a ``size`` property, then the ``size`` property is used to set the size.

.. _partition_manager_ram_configuration:

ram_size: int
   Image partitions with this property will define a Static RAM Image Partition
   Otherwise partitions with this property defines placeholder RAM partitions
   See the listing below for examples of valid use.

   .. code-block:: yaml
      :caption: Example for the ram_size property

      my_image_partition:
         ram_size: 0x1000  # <- Image 'my_image_partition' will use this for RAM
         placement:
           before: app
         size: 0x80000

      retained_log:
         ram_size: 0x2000 # <- 'retained_log' is defined as a placeholder RAM partition

All occurrences of a partition name can be replaced with a dict with the key ``one_of``, which is resolved to the first existing partition in the ``one_of`` value.
An error is raised if no partition inside the ``one_of`` dict exists.

   .. code-block:: yaml
      :caption: Example use of a ``one_of`` dict

      # Using 'one_of' in a list like this ...
      some_span:
         span: [something, {one_of: [does_not_exist_0, does_not_exist_1, exists1, exists2]}]

      # ... is equivalent to:
      some_span:
         span: [something, exists1]

      # Using 'one_of' as a dict value like this ...
      some_partition:
         placement:
            before: {one_of: [does_not_exist_0, does_not_exist_1, exists1, exists2]}

      # ... is equivalent to:
      some_partition:
         placement:
            before: exists1


.. _pm_yaml_preprocessing:

Configuration file preprocessing
================================

Each :file:`pm.yml` file is preprocessed to resolve symbols from Kconfig and DTS.

The following example is taken from the :file:`pm.yml` file for the :ref:`immutable_bootloader` provided with the  |NCS|.
It includes :file:`autoconf.h` (which is generated by Kconfig) and uses a Kconfig variable to configure the size of the ``b0`` partition.

.. code-block:: yaml

   #include <autoconf.h>
   #include <devicetree_unfixed.h>

   # 'b0' is the name of the image partition.
   b0:

     # b0 is placed before the mcuboot partition if the mcuboot partition
     # exists, otherwise it is stored before the app partition.
     placement:
       before: [mcuboot, app]
       align: {end: 0x8000}  # Align to size of SPU-lockable region.

     # The size of the b0 partition is configured in Kconfig.
     size: CONFIG_BOOTLOADER_PARTITION_SIZE

   # Don't define the provision partition if the SoC is nRF9160, because
   # the provisioning data is stored in the UICR->OTP data region.

   #ifndef CONFIG_SOC_NRF9160

   # 'provision' is the name of the placeholder partition.
   provision:
     # This partition is stored at the very end of flash.
     placement: {before: end}

   #endif /* CONFIG_SOC_NRF9160 */

.. _pm_build_system:

Build system
************
The build system finds the child images that have been enabled and their configurations.

For each image, the Partition Manager's CMake code infers the paths to the following files and folders from the name and from other global properties:

   * The :file:`pm.yml` file
   * The compiled HEX file
   * The generated include folder

After CMake finishes configuring the child images, the Partition Manager script is executed in configure time (``execute_process``) with the lists of names and paths as argument.
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


As output, the Partition Manager creates a HEX file called :file:`merged.hex`, which is programmed to the board when calling ``ninja flash``.
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
The Partition Manager stores all variables as target properties on the ``partition_manager`` target,
which means they can be used in generator expressions in the following way.

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

If the build system discovers a file named :file:`pm_static.yml` in an application's source directory, it automatically provides it to the Partition Manager script as static configuration.

The current partition configuration for a build can be found in :file:`${BUILD_DIR}/partitions.yml`.
To apply the current configuration as a static configuration, copy this file to :file:`${APPLICATION_SOURCE_DIR}/pm_static.yml`.

You can add or remove partitions as described in the following sections.

.. note::
  If the static configuration contains an entry for the ``app`` partition, this entry is ignored.

.. _ug_pm_static_remove:

Removing a static partition
---------------------------
To remove a static partition, delete its entry in :file:`pm_static.yml`.

Only partitions adjacent to the ``app`` partition or other removed partitions can be removed.

.. _ug_pm_static_add_dynamic:

Adding a dynamic partition
--------------------------
New dynamic partitions that are listed in a :file:`pm.yml` file are automatically added.
However, if a partition is defined both as static partition and as dynamic partition, the dynamic definition is ignored.

.. note::
   When resolving the relative placement of dynamic partitions, any placement properties referencing static partitions are ignored.

.. _ug_pm_static_add:

Adding a static partition
-------------------------
To add a static partition, add an entry for it in :file:`pm_static.yml`.
This entry must define the properties ``address``, ``size``, and - if applicable - ``span``.

.. code-block:: yaml
   :caption: Example of static configuration of a partition with span

   partition_name:
      address: 0xab00
      size: 0x1000
      span: [example]  # Only if this partition had the span property set originally.

.. note::
  Child images that are built with the build strategy *partition_name*\ _BUILD_STRATEGY_SKIP_BUILD or *partition_name*\ _BUILD_STRATEGY_USE_HEX_FILE must define a static partition to ensure correct placement of the dynamic partitions.
