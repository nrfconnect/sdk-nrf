.. _nrf_desktop_smp:

Simple Management Protocol module
#################################

.. contents::
   :local:
   :depth: 2

Use the Simple Management Protocol module to perform a Device Firmware Upgrade (DFU) over Bluetooth LE.

The DFU can be performed among others using the nRF Connect Android application.
The ``DFU`` button appears in the Bluetooth connected device's tab on the Android device.
After pressing the button you can select the ``*.bin`` file that will be used for the firmware update.

.. note::
  The SMP firmware update file is generated as :file:`zephyr/app_update.bin` in the build directory when building the nRF Desktop application for configuration with the MCUboot bootloader and the SMP enabled.
  In contrast to :ref:`nrf_desktop_dfu`, the SMP does not use the :file:`zephyr/dfu_application.zip` file.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_smp_start
    :end-before: table_smp_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Make sure that the following Kconfig options are enabled:

* :option:`CONFIG_MCUMGR_CMD_IMG_MGMT`
  The module uses MCUmgr image management handlers.
  See the :ref:`zephyr:device_mgmt` for details about Zephyr's device management subsystem.
* :option:`CONFIG_MCUMGR_SMP_BT`
  The device receives SMP commands over Bluetooth.
* :option:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE`
  The :ref:`nrf_desktop_dfu` performs a secondary image FLASH area erase in the background and confirms the currently running MCUboot image.
* :option:`CONFIG_BOOTLOADER_MCUBOOT`
  The DFU over Simple Management Protocol in Zephyr is supported only with the MCUboot bootloader.

Enable the module using the :option:`CONFIG_DESKTOP_SMP_ENABLE` Kconfig option.

Implementation details
**********************

During the initialization, the module registers the SMP Bluetooth service.

The module registers the :c:func:`upload_confirm` callback that is used to confirm or reject the image upload.
The image upload is rejected only if the :ref:`nrf_desktop_dfu` has called the :c:func:`dfu_lock` function.
You can find the header file of the :c:func:`dfu_lock` at the following path: :file:`src/util/dfu_lock.h`.
Before the image is received through the SMP, the Simple Management Protocol application module calls the lock function.
Also, firmware updates through the :ref:`nrf_desktop_dfu` are disabled until the nRF Desktop device is rebooted.
The modification of the data in the secondary image slot, by any other application module during the ongoing DFU, would result in a broken firmware image that would be rejected by the bootloader.

The module periodically submits ``ble_smp_transfer_event`` while the image is being uploaded.
:ref:`nrf_desktop_ble_latency` decreases the connection slave latency when it receives this event.
This is done to speed up the data transfer.
