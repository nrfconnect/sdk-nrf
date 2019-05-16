.. _ug_pm:

Partition Manager
#################

Partition Manager is a python script which sets the start address and size of all image partitions in a multi image build context.
See :ref:`application` for a description of multi image building.
An image partition is the flash area reserved for an image, to which the image binary is written.
The start address and size of partitions not containing images are also set.
The values set by Partition Manager are exposed through generated files.

Partition Manager is only invoked in a multi image build and will not be enabled in a single image build.

.. _pm_overview:

Overview
=============

The Partition Manager script reads a set of Partition Manager configurations.
Using this information it deduces the start address and size of each partition.

In a multi image build context there exists one root image, and one or more sub-images.
The size of the root image partition is dynamic, while the sizes of all sub-image partitions are statically defined.

When the start addresses and sizes have been set, the values are used in the preprocessing of the linker script for each image.

You can define three types of partitions:
 * *Image partitions* are used to set the start address and size in the linker script of an image.
 * *Placeholder partitions* reserve space like image partitions.
   However, their start address and size are not used as input to any linker script.
 * *Phony partitions* are used to get a reference to a flash area.
   They do not reserve space.
   A *Sub partition* is an example of a phony partition.

.. _pm_configuration:

Configuration
=============
Each sub-image is required to define its Partition Manager configuration in a file called :file:`pm.yml`.
This file must be stored in the same folder as the :file:`CMakeLists.txt` of that sub-image.

.. note::
   :file:`pm.yml` is only used for sub images.
   Hence, the root application does not have to define its own :file:`pm.yml` because its partition size
   and placement is implied by the size and placement of the sub image partitions.
   If a root application defines a :file:`pm.yml` it will be silently ignored.

.. _pm_yaml_format:

Configuration file format
~~~~~~~~~~~~~~~~~~~~~~~~~

The format of the :file:`pm.yml` file is as follows:

.. code-block:: yaml

   partition_name:
      option_dict_name:
         option_specific_values

Where ``option_dict_name`` can be:

placement: dict
   The placement of the partition relative to other partitions, the end of flash, or the root image partition ``app``.
   A partition with the placement property set is either an *image partition* or a *placeholder partition*.
   The partition with the same name as the image is the image partition; all the others are placeholder partitions.
   It is required that each :file:`pm.yml` defines exactly one *image partition*.
   The placement is formatted as a yaml dict.
   The valid keywords are listed below.

      before: list
         Place the partition before the first existing partition in the list.

      after: list
         Place the partition after the first existing partition in the list.

   The valid values in the lists are ``app``, ``start``, ``end``, or the name of any partition.
   It is not possible to place the partition after ``end`` or before ``start``.


span: list
   This property lists what partitions this partition should span across.
   Partitions which have the ``span`` property set are *phony partitions*,
   This property cannot be used together with the ``placement`` property.
   Non-existing partitions are ignored.
   The Partition Manager will fail if a span is empty, or the partitions are not consecutive after processing.

   .. note::
      Since configurations with ambiguous ordering are allowed, the Partition Manager may fail to find a solution,
      even if one is theoretically possible.
      I.e. the Partition Manager always detects unsatisfiable configuration (no false positives),
      but may fail on some valid inputs (some false negatives).
      Also, different versions of the script may produce different ordering.

   Below are some examples of valid and invalid configurations.

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
      :caption: Span example 3 (ambiguous order, and cannot work):

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
   The inverse of ``span``.
   ``partition_name`` will be added to the ``span`` list of the first existing partition in this list.

.. code-block:: yaml

   mcuboot:
      inside: [b0]

   b0:
      span: [] # During processing, this span will contain mcuboot.

share_size: list
   The size of the current partition will be the same as the size of the
   first existing partition in this list.
   The list can contain any kind of partition.
   This property cannot be used by phony partitions.
   Note that if the target partition is the ``app`` or spans over the ``app``,
   the size will effectively be split between them, since the ``app``'s size is dynamically decided.

.. _pm_yaml_preprocessing:

Configuration file preprocessing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each :file:`pm.yml` file is preprocessed.
Symbols from Kconfig and DTS are available.
Example of preprocessing is shown below:

.. code-block:: yaml

   #include <autoconf.h>

   # 'b0' is the name of the image partition.
   b0:

     # b0 is placed before the mcuboot partition if the mcuboot partition
     # exists, otherwise it is stored before the app partition.
     placement: {before: [mcuboot, app]}

     # The size of the b0 partition is configured in Kconfig.
     size: CONFIG_BOOTLOADER_PARTITION_SIZE

   # Don't define the provision partition if the SoC is nRF9160, this because
   # the provisioning data will be stored in the UICR->OTP data region.

   #ifndef CONFIG_SOC_NRF9160

   # 'provision' is the name of the placeholder partition.
   provision:
     # This partition is stored at the very end of flash.
     placement: last

   #endif /* CONFIG_SOC_NRF9160 */

.. _pm_build_system:

Build system
============
This section describes how the Partition Manager is included by the Zephyr build system.

If one or more sub-images are included in a build, a set of properties for that sub-image is appended to a global list.

These properties are:

   * Path to :file:`pm.yml`
   * Build directory path
   * Path to generated include folder

Once CMake finishes configuring the sub-images, the Partition Manager script is executed in configure time (``execute_process``) with the aforementioned list as argument.
The configurations generated by the Partition Manager script are imported as CMake variables.
See :ref:`pm_generated_output_and_usage`.

.. _pm_generated_output_and_usage:

Generated output and usage
==========================
For each sub-image and the root app, Partition Manager generates three files, one C header file :file:`pm_config.h`, one Kconfig file :file:`pm.config`, and one YAML file :file:`pm_static.yml`.
The C header file is used in the C code while the Kconfig file is imported in CMake.
Both these files contain the start address and size of all partitions.
The Kconfig file additionally contains the build directory and generated include folder for each image.
The YAML file contains the internal state of the Partition Manager at the end of its processing.
This means it contains the merged contents of all pm.yml files, as well as their sizes and addresses,
and other info generated by the Partition Manager.

C code usage
   When Partition Manager is enabled, all source files are compiled with the define ``USE_PARTITION_MANAGER`` set to 1.
   This allows the preprocessor to choose what code to include, depending on whether or not Partition Manager is being used.

   .. code-block:: C

      #if USE_PARTITION_MANAGER
      #include <pm_config.h>
      #define NON_SECURE_APP_ADDRESS PM_APP_ADDRESS
      #else
      ...

Hex files
   Partition Manager associates 0 or 1 hex file with each partition.
   A hex file can be assigned to a partition implicitly or explicitly.
   Explicitly assigned hex files override implicitly assigned hex files.
   Image partitions and phony partitions get a hex file assigned implicitly, while placeholder partitions do not.

   Explicitly assigning a hex file to a partition is done by setting global properties in CMake.
   The names of these properties must match a specific pattern, as shown below.

   .. code-block:: cmake
      :caption: Telling Partition manager about hex files.

      set_property(
         GLOBAL PROPERTY
         app_PM_HEX_FILE # Has to match "*_PM_HEX_FILE"
         ${PROJECT_BINARY_DIR}/signed.hex
      )

      set_property(
         GLOBAL PROPERTY
         app_PM_TARGET # Has to match "*_PM_TARGET
         sign_target
      )

   Image partitions are implicitly assigned the compiled hex file, i.e. the hex file generated when building its corresponding image.
   Phony partitions are implicitly assigned the result of merging the hex files assigned to its underlying partitions.

   For example, if a bootloader needs an image partition to be cryptographically signed,
   it must explicitly assign the signed hex file to that partition.
   This is shown in the example below

   Partition Manager creates a hex file called :file:`merged.hex`.
   :file:`merged.hex` is flashed to the board when calling ``ninja flash``.
   When creating :file:`merged.hex`, all assigned hex files are included in the merge operation.
   When the hex files overlap the conflict will be resolved as follows:

      * Hex files assigned to phony partitions overwrite hex files assigned to its underlying partitions.
      * Hex files assigned to larger partitions overwrite hex files assigned to smaller partitions.
      * Explicitly assigned hex files overwrite implicitly assigned hex files.

   This effectively allows overwriting a partition's hex file by wrapping that partition in another partition,
   and reporting a hex file for the new partition.

rom_report
   When using the Partition Manager, run ``ninja rom_report`` to see the addresses and sizes of flash partitions.

CMake usage
   The CMake variables from Partition Manager are typically used through generator expressions.
   This is because these variables are made available at the end of the CMake configure stage.
   To read a Partition Manager variable through a generator expression, the variable must be assigned as a target property.
   The ``partition_manager`` target is used for this already, and should be used for additional variables.
   Once the variable is available as a target property, the value can be read through generator expressions.
   Example usage from MCUboot is shown below.

   .. code-block:: cmake
      :caption: partition_manager.cmake

      set_property(
        TARGET partition_manager
        PROPERTY MCUBOOT_SLOT_SIZE
        ${PM_MCUBOOT_PARTITIONS_PRIMARY_SIZE}
        )

   .. code-block:: none
      :caption: mcuboot/zephyr/CmakeLists.txt

      --slot-size $<TARGET_PROPERTY:partition_manager,MCUBOOT_SLOT_SIZE>

.. _ug_pm_static:

Static configuration
========================
Static configuration is a feature intended for deployed products which consists of multiple images where
only a subset of the included images will be upgradeable through firmware update mechanisms.
One example of this is a device which includes a non-upgradable first stage bootloader and an upgradeable application.
In these use cases, the images to be upgraded must be linked to the same address as that which is deployed.
This is to avoid accidental overrides and incorrect configurations.

Static configuration defines the static partitions.
The area used by the static partitions is called the **static area**.
Using static configuration, it is still allowed to configure partitions inside the **dynamic area**.
The dynamic area consists of the "app" partition and all memory adjacent to the "app" partition which is not occupied by a static partition.
Note that there exists only one dynamic area.
When Partition Manager is executed, it only operates on the dynamic area, assuming that all other memory is reserved.
Within the dynamic area, it is allowed to define new partitions.
The dynamic area is re-sized by removing partitions from or adding partitions to the static configuration.

.. _ug_pm_static_providing:

Providing static configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The static configuration is provided through a YAML formatted configuration file.
This file is similar to the regular :file:`pm.yml` configuration files, except
that it also defines the start address for all partitions.
The given configuration for a build can be found in :file:`${BUILD_DIR}/partitions.yml`.
To apply the current configuration as a static configuration, copy this file to `${APPLICATION_SOURCE_DIR}/pm_static.yml``.

.. note::
  Configuration of the `app` partition inside the static configuration is ignored.

When the build system sees a file named :file:`pm_static.yml` in an applications source directory, it is automatically provided to the partition manager script as the static configuration.
If desired, the file can be modified as described below.

.. _ug_pm_static_remove:

Removing a static partition
~~~~~~~~~~~~~~~~~~~~~~~~~~~
A static partition is removed by deleting its entry in :file:`pm_static.yml`.
Only partitions adjacent to the ``app`` partition or other removed partitions can itself be removed.

.. _ug_pm_static_add_dynamic:

Adding a dynamic partition
~~~~~~~~~~~~~~~~~~~~~~~~~~
New dynamic partitions are added automatically by the build process if they are listed in a :file:`pm.yml` file, unless also listed in the static configuration.

.. note::
   When resolving the relative placement of dynamic partitions any placement
   properties referencing static partitions will be ignored.

.. _ug_pm_static_add:

Adding a static partition
~~~~~~~~~~~~~~~~~~~~~~~~~
To add a new static partition, add an entry for it in the file :file:`pm_static.yml`.
This entry must define the properties ``address``, ``size``, and if applicable - ``span``.

.. code-block:: yaml
   :caption: Example of static configuration of partition with span.

   partition_name:
      address: 0xab00
      size: 0x1000
      span: [example]  # Only if this partition had the `span` property set originally.

.. note::
  Sub-images with the build strategy `*_BUILD_STRATEGY_SKIP_BUILD` or
  `*_BUILD_STRATEGY_USE_HEX_FILE` must define a static partition to ensure correct
  placement of the dynamic partitions.

