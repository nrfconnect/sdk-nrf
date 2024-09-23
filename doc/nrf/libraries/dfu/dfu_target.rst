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

Common application flow
=======================

Each of the supported upgrade types follow the common DFU flow.

1. Initialize the specific DFU target by calling the :c:func:`dfu_target_init` function.

   Internally, it uses the image type and image number to decide which target to initialize.
#. Ask the offset of already downloaded blocks by calling the :c:func:`dfu_target_offset_get` function.

   This step is optional, but it allows the application to skip the blocks that are already downloaded.
   All targets do not necessarily support it.
#. Write a block of DFU image into the target by calling the :c:func:`dfu_target_write` function.

   Repeat until all blocks have been downloaded.
#. When all downloads have completed, call the :c:func:`dfu_target_done` function to tell the DFU library that the process has completed.
#. When the application is ready to install the image, call the :c:func:`dfu_target_schedule_update` function to mark it as ready for update.

   Some targets perform the installation automatically on next boot.

To cancel an ongoing operation, call the :c:func:`dfu_target_reset` function.
This clears up any images that have already been downloaded or even marked to be updated.
This is different than aborting a download by calling the :c:func:`dfu_target_done` function, in which case the same download can be resumed later by initializing the same target.

Supported upgrade types
=======================

The DFU target library supports the following types of firmware upgrades:

* MCUboot-style upgrades
* Modem delta upgrades
* Full modem firmware upgrades
* SUIT-style upgrades

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

After that, the application can call the :c:func:`dfu_target_init` function for another image pair index.

.. note::
   The application can schedule the upgrade of all the image pairs at once using the :c:func:`dfu_target_schedule_update` function.

Modem delta upgrades
--------------------

This type of firmware upgrade is used for delta upgrades to the modem firmware (see: :ref:`nrf_modem_delta_dfu`).
The modem stores the data in the memory location reserved for firmware patches.
If there is already a firmware patch stored in the modem, the library requests the modem to delete the old firmware patch to make space for the new patch.

When the transfer has completed, the application must call the :c:func:`dfu_target_done` function to release modem resources and then call :c:func:`dfu_target_schedule_update` to request the modem to apply the patch.
On the next reboot, the modem tries to apply the patch.

If an existing image needs to be removed, even if it is marked to be updated, the application may call the :c:func:`dfu_target_reset` function, which erases the DFU area and prepares it for next download.

.. _lib_dfu_target_full_modem_update:

Full modem upgrades
-------------------

.. note::
   An |external_flash_size| is required for this type of upgrade.

This type of firmware upgrade supports updating the modem firmware using the serialized firmware bundled in the zip file of the modem firmware release.
The serialized firmware file uses the :file:`.cbor` extension.

This DFU target downloads the serialized modem firmware to an external flash memory.
Once the modem firmware has been downloaded, the application should use :ref:`lib_fmfu_fdev` to write the firmware to the modem.
The DFU target library does not perform the upgrade and calling the :c:func:`dfu_target_schedule_update` function has no effect.

.. _lib_dfu_target_suit_style_update:

SUIT-style upgrades
-------------------

SUIT-style firmware upgrades can be used for :ref:`ug_nrf54h20_suit_dfu`.
Depending on the image number and the used SUIT system configuration, this type of DFU writes the data provided to the :c:func:`dfu_target_write` function into the following partitions:

* Image 0: ``dfu_partition``
* Image 1: ``dfu_cache_partition_0`` - always located just after the ``dfu_partition`` partition.
* Image 2..n: ``dfu_cache_partition_n`` - located in internal or external memory.

Before calling the :c:func:`dfu_target_init` and :c:func:`dfu_target_write` functions, the application must call the :c:func:`dfu_target_suit_set_buf` function to allocate the buffer used during the firmware update process.
The buffer size must be at least the size of the largest chunk that will be downloaded at a single time.
The buffer will be used for processing all images, so there is no need to allocate a new buffer for each image.

You can upgrade your device in several ways depending on your SUIT system configuration:

.. tabs::

   .. tab:: SUIT single image processing

      SUIT minimal processing is used for devices that do not have a cache partition.
      To enable it, set the :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_MINIMAL` Kconfig option to ``y``.

      In this approach, the SUIT envelope contains the manifests and the firmware image.
      The SUIT envelope is stored in the ``dfu_partition`` partition.
      After that, the ``dfu_cache_partition_0`` partition will be created automatically just after the ``dfu_partition`` partition and will contain the firmware.

   .. tab:: SUIT cache processing

      The SUIT cache processing requires one of the following SUIT system configurations set to ``y``:

      * :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_FULL`
      * :kconfig:option:`SUIT_DFU_CANDIDATE_PROCESSING_PUSH_TO_CACHE`

      With one of these options set, the DFU target SUIT library can process the SUIT envelope and cache images.
      In this approach, the SUIT envelope contains the manifests only, while the firmware is stored in the cache images.

      You can disable cache processing by setting the :kconfig:option:`DFU_TARGET_SUIT_CACHE_PROCESSING` Kconfig option to ``n``.

      When this approach is used, the SUIT manifests will be stored in the ``dfu_partition`` partition.
      The firmware will be stored in the ``dfu_cache_partition_1`` partition.
      This approach can be used for devices that have defined the ``dfu_cache_partition_1`` partition in the internal or external flash memory.
      The :ref:`lib_dfu_multi_image_suit_multi_image_package` uses this approach for multi-image updates.

      To read more about SUIT cache processing, see the :ref:`ug_nrf54h20_suit_external_memory` guide.

Make sure the application calls the :c:func:`dfu_target_init` function for image 0 first and then downloads the SUIT envelope.
When the single image data transfer is completed, the application using the DFU target library must call the :c:func:`dfu_target_done` function for each subsequent image.
After that, the application can call the :c:func:`dfu_target_init` function for another image index.

When all image data transfers are completed, the application using the DFU target library must do the following:

1. Call the :c:func:`dfu_target_schedule_update` function to inform SUIT that the manifests can be processed.
2. Automatically reboot the device by calling :c:func:`dfu_target_suit_reboot` with a defined delay.
   You can set the delay before rebooting the device by configuring the :kconfig:option:`CONFIG_DFU_TARGET_SUIT_REBOOT_DELAY` Kconfig option.
   Alternatively, you can skip this step and reboot the device manually.

.. note::
   The application must schedule the upgrade of all images at once using the :c:func:`dfu_target_schedule_update` function.
   During this operation, the manifests stored in the ``dfu_partition`` partition will be processed.

Configuration
*************

Configuring the library requires making edits to your component and using Kconfig options.

Enabling the library
====================

Every supported DFU target must implement the set of functions defined in the :file:`subsys/dfu/src/dfu_target.c` file.

When initializing the DFU target library, you must provide information about the type of firmware upgrade.
To do this automatically, send the first fragment of the firmware to the :c:func:`dfu_target_img_type` function.
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
Enable the following options:

* :kconfig:option:`CONFIG_SETTINGS`
* :kconfig:option:`CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS`.

The MCUboot target will then use the :ref:`zephyr:settings_api` subsystem in Zephyr to store the current progress used by the :c:func:`dfu_target_write` function across power failures and device resets.

Using a dedicated partition for full modem upgrades
===================================================

External flash partitions require the chosen node ``nordic,pm-ext-flash`` to be set to the external flash device in devicetree, see :ref:`partition_manager` for details.
When ``nordic,pm-ext-flash`` is set, the :ref:`partition_manager` will add the partition ``fmfu_storage`` to the external flash region.


API documentation
*****************

| Header file: :file:`include/dfu/dfu_target.h`
| Source files: :file:`subsys/dfu/dfu_target/src/`

.. doxygengroup:: dfu_target
