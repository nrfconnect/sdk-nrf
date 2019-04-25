.. _ug_pm:

Partition Manager
#################

Partition Manager is a python script which sets the start address and size of all image partitions in a multi image build context.
An image partition is the flash area reserved for an image, to which the image binary is written.
The start address and size of partitions not containing images are also set.
The values set by Partition Manager are exposed through generated files.

Partition Manager is only invoked in a multi image build and will not be enabled in a single image build.

.. _pm_overview:

Overview
=============

The Partition Manager script reads a set of Partition Manager and Kconfig configurations.
Using this information it deduces the start address and size of each partition.

In a multi image build context there exist one root image, and one or more sub-images.
The size of the root image partition is dynamic, while the size of all sub-image partitions are statically defined.

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

.. note::
   The root application does not define its own :file:`pm.yml` because its partition size
   and placement is implied by the size and placement of the sub image
   partitions.

.. _pm_yaml_format:

Configuration file format
~~~~~~~~~~~~~~~~~~~~~~~~~

The format of the :file:`pm.yml` file is as follows:

.. code-block:: yaml

   partition_name:
      option_dict_name:
         option_specific_values


Where ``option_dict_name`` can be:

placement: dict{}
   The placement of the partition relative to other partitions, the end of flash, or the root image partition `app`.
   The placement is formatted as a yaml dict.
   The valid keywords are listed below.

      before: list
         Place the partition before the first existing partition in the list.

      after: list
         Place the partition after the first existing partition in the list.

   The valid values in the lists are `app`, `end`, or the name of any partition.
   It is not possible to place the partition after `end`.

sub_partitions: list
   Define sub partitions which span across one or more other partitions.
   The sub partitions split the total size of the spanned partitions, and therefore have equal size.
   This property cannot be used together with the `placement` property.
   When this property is set for a partition, it is required that the `span` property is also set.

span: list
   When creating sub_partitions, this option lists what partitions the sub_partitions should span across.
   This property cannot be used together with the `placement` property.

.. _pm_yaml_preprocessing:

Configuration file preprocessing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each :file:`pm.yml` file is preprocessed.
Example of preprocessing is shown below:

.. code-block:: yaml

   #include <autoconf.h>

   # 'b0' is the name of the image partition.
   b0:

     # b0 is placed before the mcuboot partition if the mcuboot partition
     # exists, otherwise it is stored before the app partition.
     placement: {before: [mcuboot, app]}

   # Don't define the provision partition if the SoC is nRF9160, this because
   # the provisioning data will be stored in the UICR->OTP data region.

   #ifndef CONFIG_SOC_NRF9160

   # 'provision' is the name of the placeholder partition.
   provision:
     # This partition is stored at the very end of flash.
     placement: last

   #endif /* CONFIG_SOC_NRF9160 */

.. _pm_yaml_partition_types:

Partition types
~~~~~~~~~~~~~~~

It is required that each :file:`pm.yml` defines exactly one *image partition*.
This is done by using the same name for the partition as the image name.

All other partitions which have the `placement` property set are *placeholder
partitions*.

Partitions which have the `sub_partitions` property set are *phony partitions*,
and do not occupy space in flash.

In addition to the configuration provided in :file:`pm.yml`, each partition with the `placement` property set must have a corresponding size configuration in the image's ``Kconfig`` file.
The format of this configuration is as follows:

.. code-block:: none

   config PM_PARTITION_SIZE_[PARTITION_NAME]
   hex "Flash space reserved for [partition name]."
   default 0xD000
   help
     Flash space set aside for [partition name]. Note, the name
     of this configuration needs to match the requirements set by the
     script 'partition_manager.py'. See pm.yaml.

.. _pm_build_system:

Build system
============
This section describes how the Partition Manager is included by the Zephyr build system.

If one or more sub-images are included in a build, a set of properties for that sub-image is appended to a global list.

These properties are:

Path to :file:`pm.yml`
   * Build directory path
   * Path to generated include folder

Once CMake finishes configuring the sub-images, the Partition Manager script is executed in configure time (`execute_process`) with the aforementioned list as argument.
The configurations generated by the Partition Manager script are imported as CMake variables. See :ref:`pm_generated_output_and_usage`.

.. _pm_generated_output_and_usage:

Generated output and usage
==========================
For each sub-image and the root app, Partition Manager generates two files, one C header file, and one Kconfig file.
The C header file is used in the C code while the Kconfig file is imported in CMake.
Both these files contain the start address and size of all partitions.
The Kconfig file additionally contains the build directory and generated include folder for each image.

C code usage
   When Partition Manager is enabled, all source files are compiled with the define ``USE_PARTITION_MANAGER`` set to 1.
   This allows the preprocessor to choose what code to include, depending on whether or not Partition Manager is being used.

   .. code-block:: C

      #if USE_PARTITION_MANAGER
      #include <pm_config.h>
      #define NON_SECURE_APP_ADDRESS PM_APP_ADDRESS
      #else
      ...


CMake usage
   The CMake variables from Partition Manager are typically used through generator expressions.
   This is because these variables are made available at the end of the CMake configure stage.
   To read a Partition Manager variable through a generator expression, the variable must be assigned as a target property.
   The `partition_manager` target is used for this already, and should be used for additional variables.
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

