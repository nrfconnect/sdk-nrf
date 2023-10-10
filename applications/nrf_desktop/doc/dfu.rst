.. _nrf_desktop_dfu:

Device Firmware Upgrade module
##############################

.. contents::
   :local:
   :depth: 2

Use the DFU module to perform the following:

* Obtain the update image from :ref:`nrf_desktop_config_channel` and store it in the appropriate flash memory partition.
* Erase the flash memory partition in the background before storing the update image.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_dfu_start
    :end-before: table_dfu_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To perform the firmware upgrade, you must enable the bootloader.
You can use the DFU module with either MCUboot or B0 bootloader.
For more information on how to enable the bootloader, see the :ref:`nrf_desktop_bootloader` documentation.

Enable the DFU module using the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE <config_desktop_app_options>` option.
It requires the transport option :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE <config_desktop_app_options>` to be selected, as it uses :ref:`nrf_desktop_config_channel` for the transmission of the update image.

Set the value of :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_SYNC_BUFFER_SIZE <config_desktop_app_options>` to specify the size of the sync buffer (in words).
During the DFU, the data is initially stored in the buffer and then it is moved to flash.
The buffer is located in the RAM, so increasing the buffer size increases the RAM usage.
If the buffer is small, the host must perform the DFU progress synchronization more often.

.. important::
   The received update image chunks are stored on the dedicated flash memory partition when the current version of the device firmware is running.
   For this reason, make sure that you use configuration with two image partitions.
   For more information on configuring the memory layout in the application, see the :ref:`nrf_desktop_flash_memory_layout` documentation.

MCUboot bootloader mode
=======================

The MCUboot bootloader can either move the image to the primary slot before booting it (``swap mode``) or boot the image directly from the secondary slot (``direct-xip mode``).

If the MCUboot bootloader in the swap mode is selected, the DFU module does the following:

 * Requests the image upgrade after the whole image is transferred over the :ref:`nrf_desktop_config_channel`.
 * Confirms the running image after the device is rebooted.

If the MCUboot bootloader's direct-xip mode is used, the module does not mark the newly uploaded image as pending and does not confirm it after a successful boot.
In that case, the DFU module assumes that the MCUboot direct-xip bootloader simply boots an image with the higher version, so there is no need to mark the image as pending and confirm it.

The :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_MCUBOOT_DIRECT_XIP <config_desktop_app_options>` option is used to inform the DFU module that the device uses the MCUboot bootloader in the direct-xip mode.
If the option is enabled, the DFU module reports the ``MCUBOOT+XIP`` bootloader name instead of ``MCUBOOT`` to indicate that the bootloader working in the direct-xip mode is used.
The option depends on enabling the MCUboot bootloader (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`) and is enabled by default if the MCUboot direct-xip mode of operations is set (:kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP`).

.. note::
   The configured MCUboot bootloader mode needs to be manually aligned for both bootloader and application image.

Device identification information
=================================

The DFU module provides the following information about the device through the :ref:`nrf_desktop_config_channel`:

* Vendor ID (:ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_VID <config_desktop_app_options>`)
* Product ID (:ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_PID <config_desktop_app_options>`)
* Generation (:ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_GENERATION <config_desktop_app_options>`)

These values are fetched using the :ref:`devinfo <dfu_devinfo>` configuration channel option.

.. note::
   By default, the reported Vendor ID, Product ID, and generation are aligned with the values defined globally for the nRF Desktop application.
   The default values of Kconfig options used by the DFU module are based on respectively :ref:`CONFIG_DESKTOP_DEVICE_VID <config_desktop_app_options>`, :ref:`CONFIG_DESKTOP_DEVICE_PID <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_DEVICE_GENERATION <config_desktop_app_options>`.

Flash access synchronization with other DFU methods
===================================================

The DFU module leverages the :ref:`nrf_desktop_dfu_lock` to synchronize flash access with other DFU methods (for example, SMP DFU).
If multiple DFU transports are enabled in your application configuration, make sure that the following conditions are met:

* The :ref:`CONFIG_DESKTOP_DFU_LOCK <config_desktop_app_options>` option is enabled
* All of the used DFU transports use the :ref:`nrf_desktop_dfu_lock`.

On each DFU attempt, the module attempts to claim ownership over the DFU flash using the DFU Lock API.
It holds the DFU owner status until the DFU process is completed or timed out.
The lock is also kept during the background erase operation as it changes the DFU flash memory content.

If the module is not the current DFU owner and cannot claim the DFU lock, it desists from performing requested actions that either modify the DFU flash content or reboot the device.

The module also automatically starts the background erase operation of the DFU flash once it becomes the lock owner after another DFU method.

Implementation details
**********************

The DFU module implementation is centered around the transmission and the storage of the update image, and it can be broken down into the following stages:

* `Protocol operations`_ - How the module exchanges information with the host.
* `Partition preparation`_ - How the module prepares for receiving an image.

The firmware transfer operation can also be carried out either by the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr`.
The application user must not perform more than one firmware upgrade at a time.
The modification of the data by multiple application modules would result in a broken image that would be rejected by the bootloader.

Protocol operations
===================

The DFU module depends on the :ref:`nrf_desktop_config_channel` for the correct and secure data transmission between the host and the device.
The module provides a simple protocol that allows the update tool on the host to pass the update image to the device.

.. note::
   All the described :ref:`nrf_desktop_config_channel` options are used by the :ref:`nrf_desktop_config_channel_script` during the DFU.
   You can trigger DFU using a single command in the CLI.

The following :ref:`nrf_desktop_config_channel` options are available to perform the firmware update:

* :ref:`fwinfo <dfu_fwinfo>` - Passes the information about the currently executed image from the device to the host.
* :ref:`devinfo <dfu_devinfo>` - Passes the identification information about the device from the device to the host.
* :ref:`reboot <dfu_reboot>` - Reboots the device.
* :ref:`start <dfu_start>` - Starts the new update image transmission.
* :ref:`data <dfu_data>` - Passes a chunk of the update image data from the host to the device.
* :ref:`sync <dfu_sync>` - Checks the progress of the update image transmission.
* :ref:`module_variant <dfu_bootloader_var>` - Provides the host with the information about the bootloader variant used on the device.

.. _dfu_fwinfo:

fwinfo
   Perform the fetch operation on this option to get the following information about the currently executed image:

   * Version and length of the image.
   * Partition ID of the image, used to specify the image placement on the flash.

.. _dfu_devinfo:

devinfo
   Perform the fetch operation on this option to get the following information about the device:

   * Vendor ID and Product ID
   * Generation

The device generation allows to distinguish configurations that use the same board and bootloader, but are not interoperable.

.. _dfu_reboot:

reboot
   Perform the fetch operation on this option to trigger an instant reboot of the device.

   |nrf_desktop_command_note_with_dfu_lock|

.. _dfu_start:

start
   Perform the set operation on this option to start the DFU.
   The operation contains the following data:

   * Size of the image being transmitted.
   * Checksum of the update image.
   * Offset at which the host tool is going to start the image update.
     If the image transfer is performed for the first time, the offset must be set to zero.

   When the host tool performs the set operation, the checksum and the size are recorded and the update process is started.

   If the transmission is interrupted, the current offset position is stored along with the image checksum and size until the device is rebooted.
   The host tool can restart the update image transfer after the interruption, but the checksum, the requested offset, and the size of the image must match the information stored by the device.

   |nrf_desktop_command_note_with_dfu_lock|

.. _dfu_data:

data
   Perform the set operation on this option to pass the chunk of the update image that should be stored at the current offset.
   The data is initially buffered in RAM, and then it is written to the flash partition after a fetch operation is performed on the ``sync`` option.

   If the set operation on the ``data`` option is successful, the offset in buffer is increased.
   If the operation fails to be completed, the update process is interrupted.

   For performance reasons, the :ref:`nrf_desktop_config_channel` set operation does not return any information back to the host.
   To check that the update process is correct, the host tool must perform fetch operation on the ``sync`` option at regular intervals.

   |nrf_desktop_command_note_with_dfu_lock|

   .. note::
       The DFU module does not check if the update image contains any valid data.
       It also does not check if it is correctly signed.
       When the update image is received, the host tool requests a reboot.
       Then, the bootloader will check the image for validity and ensure that the signature is correct.
       If the verification of the new images is successful, the new version of the application will boot.

.. _dfu_sync:

sync
   Perform the fetch operation on this option to read the following data:

   * Information regarding the status of the update process.
     The module can report one of the following states:

     * :c:macro:`DFU_STATE_ACTIVE_OTHER` - Another DFU process is ongoing and the application is receiving the new image from the host over a different DFU transport.
     * :c:macro:`DFU_STATE_CLEANING` - The module is erasing the secondary image flash area.
     * :c:macro:`DFU_STATE_STORING` - The module is writing data to the secondary image flash area.
     * :c:macro:`DFU_STATE_ACTIVE_CONFIG_CHANNEL` - The DFU is ongoing and the module is receiving the new image from the host.
     * :c:macro:`DFU_STATE_INACTIVE` - The module is not performing any operation, and the DFU can be started.

   * Size of the update image being transmitted.
   * Checksum of the update image.
   * Offset at which the update process currently is.
   * Size of the RAM buffer used to store the data.
     The host must synchronize the firmware image transfer progress at least on every synchronization buffer byte count.

   The update tool can fetch the ``sync`` option before starting the update process to see at which offset the update is to be restarted.

   Fetching the ``sync`` option also triggers moving the image data from the RAM buffer to flash.

.. _dfu_bootloader_var:

module_variant
   Perform the fetch operation on this option to get the information about the bootloader variant that is being used on the device.


Writing data to flash
=====================

The image data that is received from the host is initially buffered in the RAM.
Writing the data to flash is triggered when the host performs the fetch operation on the ``sync`` option.
At that point, the :ref:`nrf_desktop_config_channel_script` waits until the data is written to flash before providing more image data chunks.

The data is stored in a secondary image flash partition using a dedicated work (:c:struct:`k_work_delayable`).
The work stores a single chunk of data and resubmits itself.

To ensure that the flash write will not interfere with the device usability, the stored data is split into small chunks and written only if there are no HID reports transmitted and the Bluetooth connection state does not change.

Partition preparation
=====================

The DFU module must prepare the partition before the update image can be stored.
This operation is done in the background.

To ensure that the memory erase will not interfere with the device usability, the memory pages are erased only if there are no HID reports transmitted and the Bluetooth connection state does not change.
For example, the memory is not erased right after the Bluetooth connection is established.

.. note::
    The DFU process cannot be started before the entire partition used for storing the update image is erased.
    If the DFU command is rejected, you must wait until the flash area used for the update image is erased.
