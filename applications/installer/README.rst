.. _installer:

Installer (MCUboot Firmware Loader installer)
#############################################

.. contents::
   :local:
   :depth: 2

Installer provides update functionality to the firmware loader when used with the MCUboot bootloader in firmware updater mode.
The Installer application extracts a new firmware loader image from a combined installer and firmware loader package stored in the primary slot.
It then copies the image to the firmware loader partition.
Finally, it requests firmware loader entrance on the next boot.

Overview
********

The Installer application runs from the application slot (``slot0``) when the device boots with a combined image that contains both the Installer and a new firmware loader image.
For more information, see :ref:`ug_bootloader_firmware_loader_update`.
The Installer performs the firmware loader update process as follows:

1. It reads the combined image from ``slot0_partition`` and locates the appended firmware loader image.
#. It compares the extracted image with the image currently installed in the firmware loader partition (``slot1_partition``).
#. If the images differ, it erases the firmware loader partition, copies the new image, and verifies the copied data.
#. It requests firmware loader entrance, using one of the following strategies:

   * Erase the beginning of the Installer package.
   * Request firmware loader entry via boot mode
   * Request firmware loader entry via boot request

#. It optionally reboots the device to ensure that the firmware loader runs on the next boot.

After a successful update, the device boots into the updated firmware loader instead of the Installer application.

Building and running
********************

This application is not intended to be built or programmed as a standalone application.
It is automatically included and built when one of the following sysbuild options is enabled in the project configuration:

* :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`
* :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`
* :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_UPDATE`
   This option selects :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_INSTALLER` by default, which causes this application to be built.

Testing
=======

This application is not intended for standalone testing.
It runs as part of samples that enable firmware loader update (for example, the :ref:`single_slot_sample` sample configured with the :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_UPDATE` option set to ``y``).
