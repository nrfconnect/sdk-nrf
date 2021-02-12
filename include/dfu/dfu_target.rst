.. _lib_dfu_target:

DFU target
##########

.. contents::
   :local:
   :depth: 2

The DFU target library provides a common API for the following types of firmware upgrades:

* An MCUboot style upgrade
* A modem delta upgrade.
* A full modem firmware upgrade.

Use this library in your component to do different types of firmware upgrades against a single interface.

When initializing the DFU target library, you must provide information about the type of firmware upgrade.
To do so automatically, send the first fragment of the firmware to the function :c:func:`dfu_target_img_type`.
This function can identify all supported firmware upgrade types.
The result of this call can then be given as input to the :c:func:`dfu_target_init` function.

.. note::
   After starting a DFU procedure for a given target, you cannot initialize a new DFU procedure with a different firmware file for the same target, until the DFU procedure has completed successfully or the device has been restarted.


Supported DFU targets
*********************

Every supported DFU target must implement the set of functions defined in :file:`subsys/dfu/src/dfu_target.c`.

The following sections describe the DFU targets that are currently supported.

MCUboot style upgrades
======================

This type of firmware upgrade can be used for application updates and updates to the upgradable bootloader.
It writes the data given to the :c:func:`dfu_target_write` function into the secondary slot specified by MCUboot's flash memory partitions.

For application updates, the new image replaces the existing application.
For bootloader updates, the new firmware is placed in the non-active partition (slot 0 or slot 1, see :ref:`upgradable_bootloader`).

.. note::
   When updating the bootloader, you must ensure that the provided bootloader firmware is linked against the correct partition.
   This is handled automatically by the :ref:`lib_fota_download` library.

When the complete transfer is done, call the :c:func:`dfu_target_done` function to mark the firmware as ready to be booted.
On the next reboot, the device will run the new firmware.

.. note::
   To maintain the writing progress in case the device reboots, enable the configuration options :option:`CONFIG_SETTINGS` and :option:`CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS`.
   The MCUboot target then uses the :ref:`zephyr:settings_api` subsystem in Zephyr to store the current progress used by the :c:func:`dfu_target_write` function across power failures and device resets.


Modem delta upgrades
====================

This type of firmware upgrade opens a socket into the modem and passes the data given to the :c:func:`dfu_target_write` function through the socket.
The modem stores the data in the memory location for firmware patches.
If there is already a firmware patch stored in the modem, the library requests the modem to delete the old firmware patch, to make space for the new patch.

When the complete transfer is done, call the :c:func:`dfu_target_done` function to request the modem to apply the patch, and to close the socket.
On the next reboot, the modem will try to apply the patch.

.. _lib_dfu_target_full_modem_update:

Full modem upgrades
===================

.. note::
   An external flash memory device of minimum 2MB is required for this target.

This type of firmware upgrade supports updating the modem firmware using the serialized firmware bundled in the zip file of the modem firmware release.
This DFU target will download the serialized modem firmware to an external flash memory, which is required for this type of upgrade.
Once the modem firmware has been downloaded, the library will use :ref:`lib_fmfu_fdev` to write the firmware to the modem.

Configuration
*************

You can disable support for specific DFU targets with the following parameters:

* :option:`CONFIG_DFU_TARGET_MCUBOOT`
* :option:`CONFIG_DFU_TARGET_MODEM_DELTA`
* :option:`CONFIG_DFU_TARGET_FULL_MODEM`

API documentation
*****************

| Header file: :file:`include/dfu/dfu_target.h`
| Source files: :file:`subsys/dfu/dfu_target/src/`

.. doxygengroup:: dfu_target
   :project: nrf
   :members:
