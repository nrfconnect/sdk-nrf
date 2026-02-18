.. _ug_nrf54h20_partitioning_merged:

Configuring nRF54H20 applications for updates using a merged slot
#################################################################

.. contents::
   :local:
   :depth: 2

This guide describes the merged slot update strategy.
It provides guidelines for partitioning memory and lists the steps required to configure an existing application for the nRF54H20 SoC to use a merged slot for updates in one of the Direct XIP modes.

It is organized into the following sections:

* An overview that summarizes the merged slot memory map.
* Guidelines that explain how to convert an existing application to use a merged slot.
* An example that demonstrates updates using a merged slot in Direct XIP mode with revert.

For more details on the differences between update swap and direct XIP strategies, see :ref:`ug_bootloader_main_config`.

Overview
********

The main objective of the merged slot strategy is to provide a robust and easy way to integrate firmware update scheme for multi-core SoCs like nRF54H20.
This approach simplifies firmware distribution and removes the necessity to implement dependency management between multiple images.

MCUboot multi-core support assumes that each core has an independent firmware image.
In this configuration, the non-volatile memory is partitioned into multiple slots, defining two slots for each core.
Each slot contains a firmware binary that is wrapped with MCUboot metadata.
The metadata consists of the following:

* An image header located before the firmware binary
* An image trailer located after the binary.
  The image trailer contains the update flags used by MCUboot during the firmware update process.

Although the images are treated as independent during update and verification process, the bootloader starts only the application core firmware.
The application core firmware then starts the appropriate radio core firmware.
However, MCUboot provides no signaling mechanism to pass the verification result of the radio core firmware to the application core firmware.

This limitation is not an issue when you use a swap update strategy.
Swap strategies assume that each core starts from its corresponding primary slot.

You can ensure firmware compatibility by defining mutual version dependencies such that each image depends on the other images with an identical version requirement.
This configuration satisfies the dependency checks only when all images share the same firmware version.

If any core lacks a valid firmware image, the device is non-bootable.

When using the direct XIP update strategy, the bootloader picks either of the two valid images for each core independently.
This can lead to a situation where the bootloader starts the application core firmware from the primary slot, but the radio core firmware must start from the secondary slot.
Without a signaling mechanism, the application core firmware cannot determine which radio core firmware to start, possibly booting an incompatible or unverified radio firmware.

You can avoid this issue by simplifying the bootloader logic, merging both firmwares and present them to MCUboot as a single image.
In this approach, all firmware components are first merged together and then wrapped with metadata afterwards.
This allows the bootloader to always verify and start both cores from the same slot, either primary or secondary.
This approach is called the *merged slot* update strategy.

Using the merged slot strategy has the following advantages:

* It simplifies the bootloader logic because MCUboot handles a single image.
* It reduces the risk of incompatibility between cores because you sign, verify, and update both firmwares together.
* It simplifies the update and deployment process because you deploy only one image.
* It simplifies firmware version management because the application uses a single versioning scheme.
* It reduces memory overhead because only one set of MCUboot metadata is required for the merged image.
* It hides the split between cores from the bootloader, which lets you move the boundary between cores across updates.
* It allows you to enable a slot preference mechanism, which lets the application request the slot to boot on the next reboot.

However, using the merged slot strategy has also the following disadvantages:

* It reduces flexibility because you must update both firmwares together.
* It increases the update image size, which can significantly increase update time.
* It requires adjacent primary slots and adjacent secondary slots so that the images can be merged.
* If a gap exists between the two firmwares in memory, the update image must include padding bytes, which increases the image size.
* It does not support partial updates for individual cores.
* It does not support building individual core images as standalone deliverables because you always merge the images.
* It requires an application-specific, non-default partitioning of non-volatile memory that applies to all images in the build.
  This can be challenging to implement.
* It requires a more complex build process because it merges the two firmwares before it wraps the result with MCUboot metadata.

Guidelines for enabling merged slot updates in an existing application
**********************************************************************

On the nRF54H20 SoC, the merged slot update strategy is currently supported only in the Direct XIP and firmware loader (single slot) mode.
If your application uses a different update strategy, refer to :ref:`ug_bootloader_main_config` for instructions on how to switch to Direct XIP mode.
Both Direct XIP modes, with revert and without revert, support the merged slot strategy.

To convert an existing nRF54H20 application to use the merged slot update strategy, follow these steps:

1. Create a common memory map devicetree fragment file (for example, :file:`sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi`) that redefines all of the code as well as MCUboot partitions.
   This file must be included by all the other images, providing a common memory layout.

   The memory map must include the following:

   a. The following MCUboot partitions: ``slot0_partition`` and ``slot1_partition``.

      The size of these partitions must be equal to or greater than the combined size of the application core and radio core partitions, including the MCUboot metadata.

   #. The following application core partitions: ``cpuapp_slot0_partition`` and ``cpuapp_slot1_partition``.

      Those partitions must be defined within the respective MCUboot partitions.
      There must be a 2048-byte gap between the MCUboot (merged) partition and the application core partitions to accommodate the MCUboot header.
      The header is appended by the build system during the final image creation.

   #. The following radio core partitions: ``cpurad_slot0_partition`` and ``cpurad_slot1_partition``.

      Those partitions must also be defined within the respective MCUboot partitions.
      The gap between radio and application partitions is optional and depends on the memory layout of your application.

      Since the radio partition is the last partition inside the merged MCUboot partition, you must reserve a gap within it for the MCUboot trailer.
      The trailer size is normally calculated automatically and subtracted from the available partition size through linker scripts.

      To account for the trailer in the radio partition, include the radio image name (for example, ``ipc_radio`` and ``ipc_radio_secondary_app``) in the :kconfig:option:`SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO` option.
      If the image is not included in this Kconfig option, the linker will allow to use the full partition size, leading to an unexpected update candidate rejection by the MCUboot.

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

#. Include the devicetree overlay file of the modified memory map in all the images (bootloader, application core, and radio core).

   To do so, you can define an overlay file under :file:`sysbuild/<image_name>.overlay` for each image.

   In the radio image overlay file (for example, :file:`sysbuild/ipc_radio.overlay` for the ``ipc_radio`` image) add the following:

   .. code-block:: dts

      /* Include the common memory map overlay */
      #include "nrf54h20dk_nrf54h20_memory_map.dtsi"

   The main image (the application core, usually) must define the overlay under :file:`boards/<board_name>.overlay`.

   In the application image overlay file (that is :file:`boards/<board_name>.overlay`) add the following:

   .. code-block:: dts

       /* Include the common memory map overlay */
       #include "../sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi"

   The bootloader must also be aware of the new partitions.
   This awareness can be configured using the bootloader image overlay file (that is :file:`sysbuild/mcuboot.overlay`):

   .. code-block:: dts

      /* Include the common memory map overlay */
      #include "nrf54h20dk_nrf54h20_memory_map.dtsi"

#. Configure the ``zephyr,code-partition`` property in the devicetree overlay for the bootloader to the first partition in the memory.

   Even if this value does not change, providing an overlay file for the bootloader with the new memory map has the side effect of removing the default :file:`app.overlay` from the bootloader build, which causes the bootloader code partition to be misconfigured.

   In the bootloader devicetree overlay file (that is :file:`sysbuild/mcuboot.overlay`) add the following:

   .. code-block:: dts

      / {
              chosen {
                      zephyr,code-partition = &boot_partition;
              };
      };


#. Configure the ``zephyr,code-partition`` property in the devicetree overlay for each image core to point to the new partition defined in the common memory map file.

   In the application core devicetree overlay file (that is :file:`boards/<board_name>.overlay`) add the following:

   .. code-block:: dts

      / {
              chosen {
                      zephyr,code-partition = &cpuapp_slot0_partition;
              };
      };

   And in the radio core devicetree overlay file (for example, :file:`sysbuild/ipc_radio.overlay` for the ``ipc_radio`` image) add the following:

   .. code-block:: dts

      / {
              chosen {
                      zephyr,code-partition = &cpurad_slot0_partition;
              };
      };

#. Configure the ``zephyr,code-partition`` property for the secondary images using the ``secondary_app_partition`` partition alias.

   In the application core devicetree overlay file (that is :file:`boards/<board_name>.overlay`) add:

   .. code-block:: dts

      secondary_app_partition: &cpuapp_slot1_partition {};

   And in the radio core devicetree overlay file (for example, :file:`sysbuild/ipc_radio.overlay` for the ``ipc_radio`` image) add the following:

   .. code-block:: dts

      secondary_app_partition: &cpurad_slot1_partition {};

#. Configure which images should reserve space for the MCUboot trailer.

   To do so, define the :kconfig:option:`SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO` Kconfig symbol in the :file:`sysbuild.conf` file.
   This symbol must include the names of the last image that resides in the merged slot.

   .. code-block:: kconfig

      SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO="ipc_radio;ipc_radio_secondary_app"

#. Optionally, enforce the building of the merged binary file in the build system.

   To do so, define the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY` Kconfig option in the :file:`sysbuild.conf` file.
   This Kconfig is automatically selected if the project uses Direct XIP mode on the nRF54H20 SoC.

Updating an application using merged slot in Direct XIP mode
************************************************************

When using the merged slot update strategy, the build system provides a different set of output artifacts.
The following table summarizes the differences:

+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
|                Artifact                  |                         Default strategy                                        |                    Merged slot strategy                                 |
+==========================================+=================================================================================+=========================================================================+
| Primary application core image           | ``build/<app_name>/zephyr.bin``                                                 | ``build/<app_name>/zephyr.bin`` (the same)                              |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Primary application HEX file to program  | For Direct XIP without revert:                                                  | For Direct XIP without revert:                                          |
| a device                                 |   ``build/<app_name>/zephyr.signed.hex``                                        |   ``build/zephyr.merged.hex``                                           |
|                                          | For Direct XIP with revert:                                                     | For Direct XIP with revert:                                             |
|                                          |   ``build/<app_name>/zephyr.signed.confirmed.hex``                              |   ``build/zephyr.merged.hex``                                           |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Primary application update image         | ``build/<app_name>/zephyr.signed.bin``                                          | ``build/zephyr.signed.bin``                                             |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Primary radio core image                 | ``build/<radio_image_name>/zephyr/zephyr.bin``                                  | ``build/<radio_image_name>/zephyr/zephyr.bin`` (the same)               |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Primary radio core HEX file to program   | For Direct XIP without revert:                                                  | For Direct XIP without revert:                                          |
| a device                                 |   ``build/<radio_image_name>/zephyr/zephyr.signed.hex``                         |   None. Included in ``build/zephyr.merged.hex``                         |
|                                          | For Direct XIP with revert:                                                     | For Direct XIP with revert:                                             |
|                                          |   ``build/<radio_image_name>/zephyr/zephyr.signed.confirmed.hex``               |   None. Included in ``build/zephyr.merged.hex``                         |
+------------------------------------------+---------------------------------------------------------------------------------+-------------------------------------------------------------------------+
| Primary radio core update image          | ``build/<radio_image_name>/zephyr/zephyr.signed.bin``                           | None. Included in ``build/zephyr.signed.bin``                           |
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
   This will produce new merged update candidate artifacts, ``build/zephyr.signed.bin`` and ``build/zephyr_secondary_app.signed.bin``.

#. Identify the currently running application variant (primary/A or secondary/B).

   This can be done using SMP protocol by listing images and looking for the active image flag.
   Since application is merged with the radio image, the SMP will report a single image with two slots (if programmed).

#. Upload the opposite variant of the merged update candidate to the device using SMP protocol.

   For example, if the primary variant is currently running, upload the ``build/zephyr_secondary_app.signed.bin`` file as the update candidate.
   If the secondary variant is currently running, upload the ``build/zephyr.signed.bin`` file instead.

#. Reboot the device to apply the update.

   The bootloader will verify the merged update candidate and switch to the new variant if the verification is successful.

#. (Optional) If using Direct XIP with revert mode, confirm the new image after verifying its correct operation.

   This can be done using SMP protocol by confirming the image.
   Since the application is merged with the radio image, confirming the application image will also confirm the radio image.

   If the image is not confirmed during the next boot, the bootloader will remove the update candidate and revert to the previous variant.

#. Verify that the device is running from the new variant.

   This can be done using SMP protocol by listing images and checking the active image flag.
