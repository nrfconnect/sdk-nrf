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
* External MCU upgrades over SMP
* Modem delta upgrades
* Full modem firmware upgrades
* Custom upgrades

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

External MCU upgrades over SMP
------------------------------

The SMP DFU target updates firmware on a remote MCU that exposes an mcumgr SMP server.
It forwards the firmware data supplied through the common DFU target API to the remote device using the mcumgr image management group.

Before using the common DFU target API, call the :c:func:`dfu_target_smp_client_init` function to initialize the SMP client.
To identify an image for an SMP update, call the :c:func:`dfu_target_smp_img_type_check` function and pass the returned image type to :c:func:`dfu_target_init`.
The generic :c:func:`dfu_target_img_type` function identifies an MCUboot image as a local MCUboot-style upgrade when MCUboot target support is enabled.

The SMP DFU target supports the following transports:

* Serial (UART) - Updates an external MCU running in MCUboot serial recovery mode.
  Register a callback with the :c:func:`dfu_target_smp_recovery_mode_enable` function to reset the remote MCU into recovery mode before sending SMP commands.
* Bluetooth LE (experimental) - Updates a connected device through the SMP GATT service.
  The application must establish a connection, discover the SMP service, assign the discovered handles to a DFU SMP Client instance, and call :c:func:`dfu_target_smp_bt_transport_register`.

Enable the SMP DFU target using the :kconfig:option:`CONFIG_DFU_TARGET_SMP` Kconfig option.
Select the transport using either the :kconfig:option:`CONFIG_DFU_TARGET_SMP_TRANSPORT_SERIAL` or :kconfig:option:`CONFIG_DFU_TARGET_SMP_TRANSPORT_BT` Kconfig option.
Use the :kconfig:option:`CONFIG_DFU_TARGET_SMP_IMAGE_LIST_SIZE` Kconfig option to configure the number of remote image-state entries that the target caches.

Calling the :c:func:`dfu_target_schedule_update` function marks the uploaded image for a test upgrade.
Call :c:func:`dfu_target_smp_reboot` to reboot the remote device and apply the update.
The updated firmware on the remote device must confirm the running image, for example by calling :c:func:`boot_write_img_confirmed`, to prevent MCUboot from reverting it on the next reboot.

See the :ref:`bluetooth_central_dfu_smp` sample for an example that uses the Bluetooth LE transport.

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

Custom upgrades
---------------

This firmware upgrade supports custom updates for external peripherals or other custom firmware.
To use this feature, the application must implement the custom upgrade logic by applying the functions defined in the :file:`include/dfu/dfu_target_custom.h` file.


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
* :kconfig:option:`CONFIG_DFU_TARGET_SMP`
* :kconfig:option:`CONFIG_DFU_TARGET_MODEM_DELTA`
* :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM`
* :kconfig:option:`CONFIG_DFU_TARGET_CUSTOM`

Maintaining writing progress after reboot
=========================================

You can let the application maintain the writing progress in case the device reboots.
Enable the following options:

* :kconfig:option:`CONFIG_SETTINGS`
* :kconfig:option:`CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS`.

The MCUboot target will then use the :ref:`zephyr:settings_api` subsystem in Zephyr to store the current progress used by the :c:func:`dfu_target_write` function across power failures and device resets.

Using a dedicated partition for full modem upgrades
===================================================

To configure a dedicated storage partition for full modem firmware updates, define a fixed partition in devicetree and set the ``nordic,fmfu_storage_partition`` chosen property to point to the defined partition:

.. code-block:: devicetree

   / {
   chosen {
      nordic,fmfu_storage_partition = &fmfu_storage_partition;
   };
   };

   &your_external_flash {
   partitions {
      compatible = "fixed-partitions";
      #address-cells = <1>;
      #size-cells = <1>;

      fmfu_storage_partition: partition@0 {
         reg = <0x0 0x400000>;
      };
   };
   };


API documentation
*****************

| Header file: :file:`include/dfu/dfu_target.h`
| Source files: :file:`subsys/dfu/dfu_target/src/`

.. doxygengroup:: dfu_target
