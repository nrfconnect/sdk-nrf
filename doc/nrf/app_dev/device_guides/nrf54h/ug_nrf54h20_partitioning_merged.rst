.. _ug_nrf54h20_partitioning_merged:

Configuring nRF54H20 application for updates using a merged slot
################################################################

.. contents::
   :local:
   :depth: 2

This guide describes the merged slot update strategy, provides guidelines for partitioning memory, and lists steps for configuring an existing nRF54H20 application to use a merged slot for updates in one of the Direct XIP modes.

It is organized into several key sections:

* An overview, containing a brief summary of the merged slot memory map.
* A guideline, how to convert an existing application to use a merged slot.
* An example, demonstrating how to perform updates using a merged slot in Direct XIP mode with revert.

For more details on the differences between update swap and direct XIP strategies, see :ref:`ug_bootloader_main_config`.

Overview
********

The multi-core support in MCUboot design assumes that each core has its own independent firmware image.
In such case, the non-volatile memory is partitioned into multiple slots, defining two slots for each core.
A binary that resides in each slot is wrapped with MCUboot metadata: the image header before and the image trailer with update flags after the firmware binary.

.. figure:: images/mcuboot_default_partitions.svg
   :alt: Default partitioning for multi-core applications

Although the images are treated as independent during update and verification process, only one of them - the application firmware - is started by the bootloader.
It is the application core firmware responsibility to start the correct radio core firmware.
Unfortunately, there is no signalling mechanism to pass verification results of the radio core firmware from the bootloader to the application.

This is not an issue when one of the swap update strategies is used, because these assume that each core is started from the corresponding primary slot.

.. figure:: images/mcuboot_bootchain_swap.svg
   :alt: Bootchain of the swap update strategies for multi-core applications

The diagram below illustrates the logic of selection of the radio core slot, based on the currently running application core slot, implemented inside the :file:`zephyr/soc/nordic/nrf54h/soc.c`.

.. figure:: images/mcuboot_bootchain_selection.svg
   :alt: Selection of the radio core slot based on the application core slot

A compatibility between those firmwares can be ensured through a mutual versioning dependency scheme, where each image depends on the rest of images, forcing the newest available version for each of them.

.. figure:: images/mcuboot_dependencies.svg
   :alt: Mutual versioning dependencies between multi-core firmwares

A state of the device, in which there is no valid firmware for one of the cores, is considered as non-bootable.

However, when using the direct XIP update strategy, the bootloader picks one of the two valid images for each core independently.
This can lead to a situation, in which the bootloader starts the application core firmware from the primary slot, but the radio core firmware should be started from the secondary slot.
Without a signalling mechanism, the application core firmware cannot determine which radio core firmware to start, possibly booting an incompatible or simply unverified radio firmware.

.. figure:: images/mcuboot_bootchain_xip.svg
   :alt: Bootchain of the direct XIP update strategies for multi-core applications

One solution to this problem is to simplify the bootloader logic by merging both firmwares together and present them as a single image to the bootloader.
In this case all FW parts are first merged together and wrapped with metadata afterwards.
This way, the bootloader always verifies and starts both cores from the same slot, either primary or secondary.
This approach is called the *merged slot* update strategy.

.. figure:: images/mcuboot_merged_slot.svg
   :alt: Merged slot partitioning for multi-core applications

There are several other advantages of using the merged slot strategy:

* Simplified bootloader logic, as it only needs to handle a single image.
* Reduced risk of incompatibility between cores, as both firmwares are signed, verified and updated together.
* Simplified update and deployment process, as there is only one image to deploy to devices.
* Easier management of firmware versions, as there is a single versioning scheme for the entire application.
* Reduced memory overhead, as there is only one set of MCUboot metadata for the merged image.
* The memory split between cores is not visible to the bootloader, allowing to move the boundary between cores between updates.
* It is possible to enable a slot preference mechanism, allowing the application to request which slot should be used for booting on the next reboot.

However, there are also some disadvantages to consider:

* Reduced flexibility in updating individual cores, as both firmwares must be updated together.
* Increased size of the merged image, which may lead to significantly longer update times.
* All primary slots (as well as corresponding secondary slots) must be adjacent in memory, allowing them to be merged.
* If there is a gap between the two firmwares in memory, the update binary must contain the padding bytes, artificially increasing the size of the update.
* It is not possible to provide partial updates for individual cores.
* Building of individual core firmwares is not supported, as they are always merged together.
* It requires a non-default, application-specific partitioning of the non-volatile memory, applied to all images in the build, which may be challenging to implement.
* It uses a more complex build process, as it requires merging the two firmwares together before wrapping them with metadata.

Merged slot memory map
**********************

The following diagram illustrates the memory map of the merged slot update strategy alongside a sample DTS fragment defining the partitions:

.. figure:: images/mcuboot_merged_partitions.svg
   :alt: Merged slot memory map for nRF54H20 applications

The main application sets one of the "Main application partitions" as the ``zephyr,code-partition`` in the devicetree.
The application firmware needs to be aware of the full memory layout to perform the folllowing tasks:

* Downloading the merged update candidate as well as investigating the image state through ``slot0_partition`` or ``slot1_partition``.
* Starting the radio core firmware from the respective ``cpurad_slot0_partition`` or ``cpurad_slot1_partition``.
* Using either ``cpuapp_slot0_partition`` or ``cpuapp_slot1_partition`` as the ``zephyr,code-partition``.

The "MCUboot partitions" are the only partitions interpreted by the bootloader.

The "Radio partitions" are the only partitions interpreted by the radio core firmware.
The radio firmware sets one of them as the ``zephyr,code-partition`` in its devicetree.

Guidelines for enabling merged slot updates in an existing application
**********************************************************************

Currently the merged slot update strategy is supported only in the Direct XIP mode on nRF54H20 platform.
If your application uses a different update strategy, refer to :ref:`ug_bootloader_main_config` for instructions on how to switch to Direct XIP mode.
Both Direct XIP with and without revert modes support the merged slot strategy.

To convert an existing nRF54H20 application to use the merged slot update strategy, follow these steps:

* Create a common memory map devicetree fragment file (for example :file:`sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi`) that redefines all of the code as well as MCUboot partitions.
  This file will be included by all other images, providing a common memory layout.

  The memory map should include:

  - MCUboot partitions: ``slot0_partition`` and ``slot1_partition``.

    The size of those partitions should be equal to the combined size of both application core and radio core partitions, including the MCUboot metadata.

  - Application core partitions: ``cpuapp_slot0_partition`` and ``cpuapp_slot1_partition``.

    Those partitions should be defined within the respective MCUboot partitions.
    There must be a 2048-byte gap between the MCUboot (merged) partition and the application core partitions to accommodate the MCUboot header.
    The header will be appended by the build system during the final image creation.

  - Radio core partitions: ``cpurad_slot0_partition`` and ``cpurad_slot1_partition``.

    Those partitions should also be defined within the respective MCUboot partitions.
    The gap between radio and application partitions is optional and depends on the memory layout of your application.

    Since the radio partition is the last partition inside the merged MCUboot partition, there is a need to reserve a gap within it for the MCUboot trailer.
    Normally the trailer size is calculated automatically and substracted from the available partition size through linker scripts.

    To account for the trailer in the radio partition, include the radio image name (for example ``ipc_radio`` and ``ipc_radio_secondary_app``) in the :kconfig:option:`SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO` option.
    If the image is not included in this Kconfig, the linker will allow to use the full partition size, leading to an unexpected update candidate rejection by the MCUboot.

  Sample memory map:

    .. code-block:: dts

      /* Remove the default memory map partitions */
      /delete-node/&cpuapp_slot0_partition;
      /delete-node/&cpuapp_slot1_partition;
      /delete-node/&cpurad_slot0_partition;
      /delete-node/&cpurad_slot1_partition;

      &mram1x {
              partitions {
                      /* Merged partition used by the MCUboot (variant 0). */
                      slot0_partition: partition@40000 {
                              reg = <0x40000 DT_SIZE_K(656)>;
                      };

                      /* Application code partition (variant 0).
                       * Offset by the MCUboot header size (2048 bytes).
                       */
                      cpuapp_slot0_partition: partition@40800 {
                              reg = <0x40800 DT_SIZE_K(326)>;
                      };

                      /* Radio code partition (variant 0). */
                      cpurad_slot0_partition: partition@92000 {
                              reg = <0x92000 DT_SIZE_K(328)>;
                      };

                      /* Merged partition used by the MCUboot (variant 1). */
                      slot1_partition: partition@100000 {
                              reg = <0x100000 DT_SIZE_K(656)>;
                      };

                      /* Application code partition (variant 1).
                       * Offset by the MCUboot header size (2048 bytes).
                       */
                      cpuapp_slot1_partition: partition@100800 {
                              reg = <0x100800 DT_SIZE_K(326)>;
                      };

                      /* Radio code partition (variant 1). */
                      cpurad_slot1_partition: partition@152000 {
                              reg = <0x152000 DT_SIZE_K(328)>;
                      };
              };
      };

* Include the modified memory map devicetree overlay file in all images (bootloader, application core, radio core).

  This can be done by defining an overlay file under the :file:`sysbuild/<image_name>.overlay` for each image.

  In the radio image overlay file (i.e. :file:`sysbuild/ipc_radio.overlay`) add:

  .. code-block:: dts

      /* Include the common memory map overlay */
      #include "nrf54h20dk_nrf54h20_memory_map.dtsi"

  The main image (usuallly the application core) should define the overlay under the :file:`boards/<board_name>.overlay`.

  In the application image overlay file (i.e. :file:`boards/<board_name>.overlay`) add:

  .. code-block:: dts

      /* Include the common memory map overlay */
      #include "../sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi"

  The bootloader should be also aware of the new partitions.
  This can be configured using the bootloader image overlay file (i.e. :file:`sysbuild/mcuboot.overlay`):

  .. code-block:: dts

      /* Include the common memory map overlay */
      #include "nrf54h20dk_nrf54h20_memory_map.dtsi"

* Configure the ``zephyr,code-partition`` property in the devicetree overlay for the bootloader to the first partition in the memory.

  Although this value does not change, providing an overlay file for the bootloader with the new memory map removed the default :file:`app.overlay` from the bootloader build,
  leaving the bootloader code partition misconfigured.

  In the bootloader devicetree overlay file (i.e. :file:`sysbuild/mcuboot.overlay`) add:

  .. code-block:: dts

    / {
            chosen {
                    zephyr,code-partition = &boot_partition;
            };
    };


* Configure the ``zephyr,code-partition`` property in the devicetree overlay for each image core to point to the new partition defined in the common memory map file.

  In the application core devicetree overlay file (i.e. :file:`boards/<board_name>.overlay`) add:

  .. code-block:: dts

    / {
            chosen {
                    zephyr,code-partition = &cpuapp_slot0_partition;
            };
    };

  And in the radio core devicetree overlay file (i.e. :file:`sysbuild/ipc_radio.overlay`) add:

  .. code-block:: dts

    / {
            chosen {
                    zephyr,code-partition = &cpurad_slot0_partition;
            };
    };

* Configure the ``zephyr,code-partition`` property for the secondary images using the ``secondary_app_partition`` partition alias.

  In the application core devicetree overlay file (i.e. :file:`boards/<board_name>.overlay`) add:

  .. code-block:: dts

      secondary_app_partition: &cpuapp_slot1_partition {};

  And in the radio core devicetree overlay file (i.e. :file:`sysbuild/ipc_radio.overlay`) add:

  .. code-block:: dts

      secondary_app_partition: &cpurad_slot1_partition {};

* Configure, which images should reserve space for the MCUboot trailer.

  This can be done by defining the :kconfig:option:`SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO` Kconfig symbol in the :file:`sysbuild.conf` file.
  This symbol should include the names of the last image that reside in the merged slot.

  .. code-block:: kconfig

      SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO="ipc_radio;ipc_radio_secondary_app"

* Optionally enforce building the merged binary in the build system.

  This can be done by enabling the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY` Kconfig option in the :file:`sysbuild.conf` file.
  This Kconfig should be automatically selected if the project uses Direct XIP mode on nRF54H20 platform.

Updating an application using merged slot in Direct XIP mode
************************************************************

When using the merged slot update strategy, the build system provides a different set of output artifacts.
The table below summarizes the differences:

+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
|                Artifact                  |                         Default strategy                                        |                    Merged slot strategy                                 |
+==========================================+=================================================================================+=========================================================================+
| Application core image                   | ``build/<app_name>/zephyr.bin``                                                 | ``build/<app_name>/zephyr.bin`` (the same)                              |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Application HEX file to program a device | For Dirext XIP without revert:                                                  | For Dirext XIP without revert:                                          |
|                                          |   ``build/<app_name>/zephyr.signed.hex``                                        |   ``build/zephyr.merged.hex``                                           |
|                                          | For Direct XIP with revert:                                                     | For Direct XIP with revert:                                             |
|                                          |   ``build/<app_name>/zephyr.signed.confirmed.hex``                              |   ``build/zephyr.merged.hex``                                           |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Application update image                 | ``build/<app_name>/zephyr.signed.bin``                                          | ``build/zephyr.signed.bin``                                             |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Radio core image                         | ``build/<radio_image_name>/zephyr/zephyr.bin``                                  | ``build/<radio_image_name>/zephyr/zephyr.bin`` (the same)               |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Radio core HEX file to program a device  | For Direct XIP without revert:                                                  | For Direct XIP without revert:                                          |
|                                          |   ``build/<radio_image_name>/zephyr/zephyr.signed.hex``                         |   None. Included in ``build/zephyr.merged.hex``                         |
|                                          | For Direct XIP with revert:                                                     | For Direct XIP with revert:                                             |
|                                          |   ``build/<radio_image_name>/zephyr/zephyr.signed.confirmed.hex``               |   None. Included in ``build/zephyr.merged.hex``                         |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Radio core update image                  | ``build/<radio_image_name>/zephyr/zephyr.signed.bin``                           | None. Included in ``build/zephyr.signed.bin``                           |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Secondary application core image         | ``build/mcuboot_secondary_app/zephyr.bin``                                      | ``build/mcuboot_secondary_app/zephyr.bin`` (the same)                   |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Secondary application HEX file to        | For Direct XIP without revert:                                                  | For Direct XIP without revert:                                          |
| program a device                         |   ``build/mcuboot_secondary_app/zephyr.signed.hex``                             |   ``build/zephyr_secondary_app.merged.hex``                             |
|                                          | For Direct XIP with revert:                                                     | For Direct XIP with revert:                                             |
|                                          |   ``build/mcuboot_secondary_app/zephyr.signed.confirmed.hex``                   |   ``build/zephyr_secondary_app.merged.hex``                             |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Secondary application update image       | ``build/mcuboot_secondary_app/zephyr.signed.bin``                               | ``build/zephyr_secondary_app.signed.bin``                               |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Secondary radio core image               | ``build/<radio_image_name>_secondary_app/zephyr/zephyr.bin``                    | ``build/<radio_image_name>_secondary_app/zephyr/zephyr.bin`` (the same) |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Secondary radio core HEX file to         | For Direct XIP without revert:                                                  | For Direct XIP without revert:                                          |
| program a device                         |   ``build/<radio_image_name>_secondary_app/zephyr/zephyr.signed.hex``           |   None. Included in ``build/zephyr_secondary_app.merged.hex``           |
|                                          | For Direct XIP with revert:                                                     | For Direct XIP with revert:                                             |
|                                          |   ``build/<radio_image_name>_secondary_app/zephyr/zephyr.signed.confirmed.hex`` |   None. Included in ``build/zephyr_secondary_app.merged.hex``           |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Secondary radio core update image        | ``build/<radio_image_name>_secondary_app/zephyr/zephyr.signed.bin``             | None. Included in ``build/zephyr_secondary_app.signed.bin``             |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+

To perform an update using Direct XIP mode with the merged slot strategy, follow these steps:

1. Rebuild the application with increased version number.

   The image version can be increased by modifying the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig symbol in the :file:`prj.conf` file.
   This will produce a new merged update candidate artifacts: ``build/zephyr.signed.bin`` and ``build/zephyr_secondary_app.signed.bin``.

#. Identify the currently running application variant (primary/A or secondary/B).

   This can be done using SMP protocol by listing images and looking for the active image flag.
   Since application is merged with the radio image, the SMP will report a single image with two slots (if programmed).

#. Upload the opposite variant of the merged update candidate to the device using SMP protocol.

   For example, if the primary variant is currently running, upload the ``build/zephyr_secondary_app.signed.bin`` file as the update candidate.

#. Reboot the device to apply the update.

   The bootloader will verify the merged update candidate and switch to the new variant if the verification is successful.

#. (Optional) If using Direct XIP with revert mode, confirm the new image after verifying its correct operation.

   This can be done using SMP protocol by confirming the image.
   Since the application is merged with the radio image, confirming the application image will also confirm the radio image.

   If the image is not confirmed during the next boot, the bootloader will remove the update candidate and revert to the previous variant.

#. Verify that the device is running from the new variant.

   This can be done using SMP protocol by listing images and checking the active image flag.
