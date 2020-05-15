.. _nrf_desktop_dfu:

Device Firmware Upgrade module
##############################

The DFU module is used for obtaining the update image from a transport and storing it in the appropriate partition on the flash memory.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_dfu_start
    :end-before: table_dfu_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To perform the firmware upgrade, the bootloader must be enabled.
More on how to enabled the bootloader can be found at :ref:`nrf_desktop_bootloader`.

The DFU module is selected using ``CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE``.
Because it uses the :ref:`nrf_desktop_config_channel` for the transmission of the update image, it requires the transport option ``CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE`` to be selected.

When the update image is being received, it is stored in the dedicated partition on the flash memory.
For more information about configuring the memory layout in the application, see :ref:`nrf_desktop_flash_memory_layout`.

Implementation details
**********************

The DFU module implementation is centered around transmission and storage of the update image, and it can be broken down into the following stages:

* `Protocol operations`_ - How the module exchanges information with the host.
* `Partition preparation`_ - How the module prepares for receiving an image.

Protocol operations
===================

The DFU module depends on the :ref:`nrf_desktop_config_channel` for the correct and secure data transmission between the host and the device.
The module provides a simple protocol that allows the update tool on the host to pass the update image to the device.

The following commands are available for controlling the module operation:

* `fwinfo`_ - Pass the information about currently executed image from the device to the host.
* `reboot`_ - Reboot the device.
* `start`_ - Start the new update image transmission.
* `data`_ - Pass a chunk of the update image data from the host to the device.
* `sync`_ - Check the progress of the update image transmission.

fwinfo
------

The ``fwinfo`` command is issued by sending a ``config_fetch_request_event`` from the host tool through the DFU module, with the ``fwinfo`` option for identification.

The return data from the DFU module contains the following information about the currently executed image:

* Version and length of the image.
* Partition ID of the image, used to specify the image placement on the flash.

reboot
------

This command is issued by sending a ``config_fetch_request_event`` from the host tool through the DFU module, with the ``reboot`` option used for identification.
When received, it triggers an instant reboot of the device.

start
-----

The ``start`` command is issued by sending a ``config_event`` from the host tool through the DFU module, with the ``start`` option used for identification.
The command contains the following data:

* Update image checksum.
* Size of the image.
* Offset at which the host tool is to start an image update.
  If the image transfer is performed for the first time, the offset must be set to zero.

When the command is issued, the checksum and the size are recorded and the update process is started.

If for some reason the transmission is interrupted, the current offset position is stored along with the image checksum and size until the device is rebooted.
The host tool can restart the update image transfer after interruption.
In such case, the checksum and the size of the image (as well as the requested offset) must match the information stored by the device.

data
----

This command is issued by sending a ``config_event`` from the host tool through the DFU module, with the ``data`` option used for identification.
The command passes the chunk of the update image that should be stored at the current offset.

If the operation is successful, the offset is increased.
In case of failure the update process is interrupted.

For performance reasons, this command does not return any information back to the host.
To check that the update process is correct, the host tool must issue a ``sync`` command in regular intervals.

.. note::
    The DFU module does not check if the update image contains any valid data.
    It also does not check if it is correctly signed.
    When the update image is received, the host tool should request a reboot.
    Then the bootloader will check the image for validity and ensure the signature is correct.
    If verification of the new images is successful, the new version of the application will boot.

sync
----

The ``sync`` command is issued by sending a ``config_fetch_request_event`` from the host tool with the DFU module and the ``sync`` option used for identification.
The command confirms to the tool that the update process is ongoing.
It also sends back the following data:

* Checksum.
* Size of the update image being transmitted.
* Offset at which the update process currently is.

The update tool can issue the ``sync`` command before starting the update process to see at which offset the update is to be restarted.

Partition preparation
=====================

The DFU module must prepare the partition before the update image can be stored.
This operation is done in the background.

To ensure that the memory erase will not interfere with the device usability, the memory pages are erased only if there are no HID reports transmitted and the Bluetooth connection state does not change (for example, memory is not erased right after the Bluetooth connection is established).

.. warning::
    The DFU process cannot be started before the entire partition used for storing the update image is erased.
    If the start command is rejected, you must wait until all erase operations are completed.
