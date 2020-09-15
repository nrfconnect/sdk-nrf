.. _nrf_desktop_dfu:

Device Firmware Upgrade module
##############################

Use the DFU module to obtain the update image from :ref:`nrf_desktop_config_channel` and to store it in the appropriate partition on the flash memory.

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
For more information on how to enable the bootloader, see the :ref:`nrf_desktop_bootloader` documentation.

Enable the DFU module using the ``CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE`` option.
It requires the transport option ``CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE`` to be selected, as it uses :ref:`nrf_desktop_config_channel` for the transmission of the update image.

When the update image is being received, it is stored in the dedicated partition on the flash memory.
For more information on configuring the memory layout in the application, see the :ref:`nrf_desktop_flash_memory_layout` documentation.

Implementation details
**********************

The DFU module implementation is centered around the transmission and the storage of the update image, and it can be broken down into the following stages:

* `Protocol operations`_ - How the module exchanges information with the host.
* `Partition preparation`_ - How the module prepares for receiving an image.

The firmware transfer operation can also be carried out by :ref:`nrf_desktop_smp`.
The application module must call the :cpp:func:`dfu_lock` function before the transfer operation, as well as before erasing the flash area.
This is done to make sure that only one application module could modify the secondary image flash partition.
The modification of the data by multiple application modules would result in a broken image that would be rejected by the bootloader.
After the operation is finished, the application module can call the :cpp:func:`dfu_unlock` to let other modules modify the secondary image flash partition.

You can find the header file of the :cpp:func:`dfu_lock` and :cpp:func:`dfu_unlock` at the following path: :file:`src/util/dfu_lock.h`.

Protocol operations
===================

The DFU module depends on the :ref:`nrf_desktop_config_channel` for the correct and secure data transmission between the host and the device.
The module provides a simple protocol that allows the update tool on the host to pass the update image to the device.

.. note::
   All the described :ref:`nrf_desktop_config_channel` options are used by the :ref:`nrf_desktop_config_channel_script` during the ``dfu`` operation.
   You can trigger DFU using a single command in CLI.

The following :ref:`nrf_desktop_config_channel` options are available to perform the firmware update:

* :ref:`fwinfo <dfu_fwinfo>` - Passes the information about the currently executed image from the device to the host.
* :ref:`reboot <dfu_reboot>` - Reboots the device.
* :ref:`start <dfu_start>` - Starts the new update image transmission.
* :ref:`data <dfu_data>` - Passes a chunk of the update image data from the host to the device.
* :ref:`sync <dfu_sync>` - Checks the progress of the update image transmission.

.. _dfu_fwinfo:

fwinfo
   Perform the fetch operation on this option to get the following information about the currently executed image:

   * Version and length of the image.
   * Partition ID of the image, used to specify the image placement on the flash.

.. _dfu_reboot:

reboot
   Perform the fetch operation on this option to trigger an instant reboot of the device.

.. _dfu_start:

start
   Perform the set operation on this option to start the DFU.
   The command contains the following data:

   * Checksum of the update image.
   * Size of the image being transmitted.
   * Offset at which the host tool is to start the image update.
     If the image transfer is performed for the first time, the offset must be set to zero.

   When the host tool issues the command, the checksum and the size are recorded and the update process is started.

   If the transmission is interrupted, the current offset position is stored along with the image checksum and size until the device is rebooted.
   The host tool can restart the update image transfer after the interruption, but the checksum, the requested offset, and the size of the image must match the information stored by the device.

.. _dfu_data:

data
   Perform the set operation on this option to pass the chunk of the update image that should be stored at the current offset.

   If the operation is successful, the offset is increased.
   In the operation fails to be completed, the update process is interrupted.

   For performance reasons, this command does not return any information back to the host.
   To check that the update process is correct, the host tool must issue a ``sync`` command at regular intervals.

   .. note::
       The DFU module does not check if the update image contains any valid data.
       It also does not check if it is correctly signed.
       When the update image is received, the host tool requests a reboot.
       Then, the bootloader will check the image for validity and ensure that the signature is correct.
       If the verification of the new images is successful, the new version of the application will boot.

.. _dfu_sync:

sync
   Perform the fetch operation to read the following data:

   * Information regarding the status of the update process.
   * Checksum of the update image.
   * Size of the update image being transmitted.
   * Offset at which the update process currently is.

   The update tool can fetch the ``sync`` option before starting the update process to see at which offset the update is to be restarted.

Partition preparation
=====================

The DFU module must prepare the partition before the update image can be stored.
This operation is done in the background.

To ensure that the memory erase will not interfere with the device usability, the memory pages are erased only if there are no HID reports transmitted and the Bluetooth connection state does not change.
For example, the memory is not erased right after the Bluetooth connection is established.

.. warning::
    The DFU process cannot be started before the entire partition used for storing the update image is erased.
    If the start command is rejected, you must wait until all erase operations are completed.
