.. _caf_ble_smp:

CAF: Simple Management Protocol module
######################################

.. contents::
   :local:
   :depth: 2

The |smp| of the :ref:`lib_caf` (CAF) allows to perform the device firmware upgrade (DFU) over Bluetooth LE.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :kconfig:`CONFIG_CAF_BLE_STATE` - This module enables :ref:`caf_ble_state` module.
* :kconfig:`CONFIG_CAF_BLE_SMP` - This option enables |smp| over Bluetooth LE.
* :kconfig:`CONFIG_MCUMGR_CMD_IMG_MGMT` - This option enables MCUmgr image management handlers, which are required for the DFU process.
  For details, see :ref:`zephyr:device_mgmt` in the Zephyr documentation.
* :kconfig:`CONFIG_MCUMGR_SMP_BT` - This option enables support for the SMP commands over Bluetooth.
* :kconfig:`CONFIG_BOOTLOADER_MCUBOOT` - This option enables the MCUboot bootloader.
  The DFU over Simple Management Protocol in Zephyr is supported only with the MCUboot bootloader.

Enabling remote OS management
=============================

The |smp| supports registering OS management handlers automatically, which you can enable using the following Kconfig option:

* :kconfig:`CONFIG_MCUMGR_CMD_OS_MGMT` - This option enables MCUmgr OS management handlers.
  Use these handlers to remotely trigger the device reboot after the image transfer is completed.
  After the reboot, the device starts using the new firmware.
  One of the applications that support the remote reboot functionality is `nRF Connect for Mobile`_.

Implementation details
**********************

During the initialization, the module registers the SMP Bluetooth service, which allows to perform DFU over Bluetooth LE.

The module registers the :c:func:`upload_confirm` callback that is used to submit ``ble_smp_transfer_event``.

The application user must not perform more than one firmware upgrade at a time.
The modification of the data by multiple application modules can result in a broken image that is going to be rejected by the bootloader.

The module periodically submits ``ble_smp_transfer_event`` while the image is being uploaded.

You can perform the DFU using for example the `nRF Connect for Mobile`_ application.
The :guilabel:`DFU` button appears in the tab with the connected Bluetooth devices.
After pressing the button, you can select the :file:`*.bin` file that is to be used for the firmware update.

.. note::
  The SMP firmware update file is generated as :file:`zephyr/app_update.bin` in the build directory when building your application for configuration with the |smp| enabled.

.. |smp| replace:: simple management protocol module
