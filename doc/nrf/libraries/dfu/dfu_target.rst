.. _lib_dfu_target:

DFU target
##########

.. contents::
   :local:
   :depth: 2

The DFU target library provides an API that supports different types of firmware upgrades against a single interface.

Overview
********

The DFU target library collects sequentially the image data received from a transport protocol.
It then saves the data using buffered writes, resulting in fewer write operations and faster response times seen by the application.
The library also takes away, from the application, the responsibility of calculating the data-chunk write offset.

Supported upgrade types
=======================

The DFU target library supports the following types of firmware upgrades:

* MCUboot-style upgrades
* Modem delta upgrades
* Full modem firmware upgrades

MCUboot-style upgrades
----------------------

You can use MCUboot-style firmware upgrades for updates to the application and to the upgradable bootloader.
This type of DFU writes the data given to the :c:func:`dfu_target_write` function into the secondary slot specified by MCUboot's flash memory partitions.
Depending on the type of upgrade, it has the following behavior:

* For application updates, the new image replaces the existing application.
* For bootloader updates, the new firmware is placed in the non-active partition (slot 0 or slot 1, see :ref:`upgradable_bootloader`).

The MCUboot target type supports multiple image pairs, like the application core image pair and the network core image pair, to support multi-image updates where the device contains two or more executable images at once.

.. note::
   When updating the bootloader, ensure that the provided bootloader firmware is linked against the correct partition.
   This is handled automatically by the :ref:`lib_fota_download` library.

When the image data transfer is completed, the application using the DFU target library must do the following:

1. Call the :c:func:`dfu_target_done` function to finish the image data collection.
2. Call the :c:func:`dfu_target_schedule_update` function to mark the firmware as *ready to be booted*.
   On the next reboot, the device will run with the new firmware.

When the application has to collect another MCUboot image, it must either reset the DFU target using :c:func:`dfu_target_reset` or schedule the upgrade for the current image.
After that, the application can call the :c:func:`dfu_target_init` function for another image pair index.

.. note::
   The application can schedule the upgrade of all the image pairs at once using the :c:func:`dfu_target_schedule_update` function.

Modem delta upgrades
--------------------

This type of firmware upgrade is used for delta upgrades to the modem firmware (see: :ref:`nrf_modem_delta_dfu`).
The modem stores the data in the memory location reserved for firmware patches.
If there is already a firmware patch stored in the modem, the library requests the modem to delete the old firmware patch to make space for the new patch.

When the transfer is completed, The application must call the :c:func:`dfu_target_done` function to request the modem to apply the patch.
On the next reboot, the modem will try to apply the patch.

.. _lib_dfu_target_full_modem_update:

Full modem upgrades
-------------------

.. note::
   An |external_flash_size| is required for this type of upgrade.

This type of firmware upgrade supports updating the modem firmware using the serialized firmware bundled in the zip file of the modem firmware release.
The serialized firmware file uses the :file:`.cbor` extension.

This DFU target downloads the serialized modem firmware to an external flash memory.
Once the modem firmware has been downloaded, the library uses :ref:`lib_fmfu_fdev` to write the firmware to the modem.

Configuration
*************

Configuring the library requires making edits to your component and using Kconfig options.

Enabling the library
====================

Every supported DFU target must implement the set of functions defined in :file:`subsys/dfu/src/dfu_target.c`.

When initializing the DFU target library, you must provide information about the type of firmware upgrade.
To do so automatically, send the first fragment of the firmware to the function :c:func:`dfu_target_img_type`.
This function can identify all supported firmware upgrade types.
The result of this call can then be given as input to the :c:func:`dfu_target_init` function.

.. note::
   After starting a DFU procedure for a given target, you cannot initialize a new DFU procedure with a different firmware file for the same target until the pending DFU procedure has completed successfully or the device has been restarted.

Disabling support for specific DFU targets
==========================================

You can disable support for specific DFU targets using the following options:

* :kconfig:option:`CONFIG_DFU_TARGET_MCUBOOT`
* :kconfig:option:`CONFIG_DFU_TARGET_MODEM_DELTA`
* :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM`

Maintaining writing progress after reboot
=========================================

You can let the application maintain the writing progress in case the device reboots.
To do so, enable the following options:

* :kconfig:option:`CONFIG_SETTINGS`
* :kconfig:option:`CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS`.

The MCUboot target will then use the :ref:`zephyr:settings_api` subsystem in Zephyr to store the current progress used by the :c:func:`dfu_target_write` function across power failures and device resets.

API documentation
*****************

| Header file: :file:`include/dfu/dfu_target.h`
| Source files: :file:`subsys/dfu/dfu_target/src/`

.. doxygengroup:: dfu_target
   :project: nrf
   :members:
