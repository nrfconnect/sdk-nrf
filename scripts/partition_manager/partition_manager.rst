.. _partition_manager:

Partition Manager
#################

The Partition Manager is a Python script that sets the start address and size of all image partitions in a multi-image build context.
When creating an application that requires child images (for example, a bootloader), you can configure the Partition Manager to control where in memory each image should be placed.

See :ref:`ug_multi_image` for more information about multi-image builds.

The Partition Manager is activated for all multi-image builds, no matter what build strategy is used for the child image.

.. _pm_overview:

Overview
********

The Partition Manager script reads a set of Partition Manager configurations.
Using this information, it deduces the start address and size of each partition.

There are different kinds of partitions:

Image partitions
   An image partition is the flash area reserved for an image, to which the image binary is written.

   In a multi-image build context, there is one *root image* and one or more *child images*.
   The size of the root image partition is dynamic, while the sizes of all child image partitions are statically defined.

Placeholder partitions
   A placeholder partition does not contain an image, but reserves space for other uses.
   For example, you might want to reserve space in flash to hold an updated application image while it is being downloaded and before it is copied to the application image partition.

Container partitions
   A container partition does not reserve space, but is used to logically and/or physically group other partitions.

The start addresses and sizes for image partitions are used in the preprocessing of the linker script for each image.

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
Subsystem Partition Manager configurations cannot define image partitions.

There are some limitations when multiple images include the same subsystem which defines a Partition Manager configuration.
All partitions are global, and will only be included once, even if multiple configurations define them.
When multiple configurations define the same partition, the configuration of that partition must be identical in all definitions.
An exception is raised if the configuration differs.

The listing below shows how to properly define the Partition Manager configuration for a subsystem.
This must be placed in the :file:`CMakeLists.txt` that defines the subsystem.

.. code-block:: cmake

  add_partition_manager_config(pm.yml)

.. _pm_yaml_format:

Configuration file format
=========================

The format of the :file:`pm.yml` file is as follows:

.. code-block:: yaml

   partition_name:
      option_dict_name:
         option_specific_values

*partition_name* is the name of the partition (for example, ``mcuboot``).
*option_dict_name* and *option_specific values* can be any of the following:

placement: dict
   This property specifies the placement of the partition relative to other partitions, to the end of flash, or to the root image partition ``app``.

   A partition with the placement property set is either an image partition or a placeholder partition.
   If the partition name is the same as the image name (as defined in ``CMakeLists.txt``, see *Defining new child images* in :ref:`zephyr:application`), this partition is the image partition.
   All other partitions are placeholder partitions.
   Each :file:`pm.yml` file must define exactly one image partition.

   The placement is formatted as a YAML dict.
   The valid keywords are listed below:

      before: list
         Place the partition before the first existing partition in the list.

      after: list
         Place the partition after the first existing partition in the list.

     Valid values in the lists are ``app``, ``start``, ``end``, or the name of any partition.
     It is not possible to place the partition after ``end`` or before ``start``.

      align: dict
         Ensure alignment of start or end of partition by specifying a dict with a ``start`` or ``end`` key respectively, where the value is the number of bytes to align to.
         If necessary, empty partitions are inserted in front of or behind the partition to ensure that the alignment is correct.
         Only one key can be specified.
         Partitions which directly or indirectly (through spans) share size with the ``app`` partitions can only be aligned if they are placed directly after the ``app`` partition.


span: list OR dict: list
   If the value type is a list, this property lists which partitions this partition should span across.

   A string formatted value is interpreted as a single item list.
   Partitions with this property are container partitions.
   Therefore, this property cannot be used together with the ``placement`` property.

   Non-existing partitions are removed from the ``span`` list before processing, and partitions with empty ``span`` lists are removed altogether (unless filled via ``inside``).

   .. note::
      You can specify configurations with an ambiguous ordering (see the following examples).
      However, different versions of the script might produce a different ordering for such configurations, and the Partition Manager might fail to find a solution even if one is theoretically possible.
      The Partition Manager always detects unsatisfiable configurations (no false positives), but it might fail on some valid inputs (false negatives).

   See the following examples of valid and invalid configurations:

   .. code-block:: yaml
      :caption: Span example 1 (fixed order, cannot work)

      mcuboot:
         placement:
            before: [spm, app]

      spm:
         placement:
            before: [app]

      foo:
         span: [mcuboot, app] # This will fail, because 'spm' will be placed between mcuboot and app.

      # Order: mcuboot, spm, app

   .. code-block:: yaml
      :caption: Span example 2 (ambiguous order)

      mcuboot:
         placement:

      spm:
         placement:
            after: [mcuboot]

      app:
         placement:
            after: [mcuboot]

      foo:
         span: [mcuboot, app] # The order of spm and app is ambiguous in this case, but since
                              # this span exists, Partition Manager will try to increase the
                              # likelihood that mcuboot and app are placed next to each other.

      # Order 1: mcuboot, spm, app
      # Order 2: mcuboot, app, spm
      # The algorithm should coerce order 2 to make foo work.

   .. code-block:: yaml
      :caption: Span example 3 (ambiguous order, cannot work)

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

      # Order 1: mcuboot, spm, app
      # Order 2: mcuboot, app, spm
      # foo requires order 2, while bar requires order 1.

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

All occurrences of a partition name can be replaced with a dict with the key ``one_of``, which is resolved to the first existing partition in the ``one_of`` value.
An error is raised if no partition inside the ``one_if`` dict exists.

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

The following example shows a typical :file:`pm.yml` file.
It includes :file:`autoconf.h` (which is generated by Kconfig) and uses a Kconfig variable to configure the size of the ``b0`` partition.

.. code-block:: yaml

   #include <autoconf.h>
   #include <generated_dts_board_unfixed.h>

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

For each image, Partition Manager's CMake code infers the paths to the following files and folders from the name and from other global properties:

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
