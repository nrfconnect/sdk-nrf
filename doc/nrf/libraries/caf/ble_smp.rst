.. _caf_ble_smp:

CAF: Simple Management Protocol module
######################################

.. contents::
   :local:
   :depth: 2

The |smp| of the :ref:`lib_caf` (CAF) allows to perform the device firmware upgrade (DFU) over Bluetooth® LE.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :kconfig:option:`CONFIG_CAF_BLE_STATE` - This module enables :ref:`caf_ble_state`.
* :kconfig:option:`CONFIG_CAF_BLE_SMP` - This option enables |smp| over Bluetooth LE.
* :kconfig:option:`CONFIG_MCUMGR_GRP_IMG` - This option enables MCUmgr image management handlers, which are required for the DFU process.
  For details, see :ref:`zephyr:device_mgmt` in the Zephyr documentation.
* :kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` - This option enables MCUmgr notification hook support, which allows this module to listen for a MCUmgr event.
  For details, see :ref:`zephyr:mcumgr_callbacks` in the Zephyr documentation.
* :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK` - This option enables MCUmgr upload check hook, which sends image upload requests to the registered callbacks.
* :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` - This option enables support for the SMP commands over Bluetooth.
* :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` - This option enables the MCUboot bootloader.
  The DFU over Simple Management Protocol in Zephyr is supported only with the MCUboot bootloader.

Enabling remote OS management
=============================

The |smp| supports registering OS management handlers automatically, which you can enable using the following Kconfig option:

* :kconfig:option:`CONFIG_MCUMGR_GRP_OS` - This option enables MCUmgr OS management handlers.
  Use these handlers to remotely trigger the device reboot after the image transfer is completed.
  After the reboot, the device starts using the new firmware.
  One of the applications that support the remote reboot functionality is `nRF Connect for Mobile`_.

Implementation details
**********************

The module periodically submits :c:struct:`ble_smp_transfer_event` during the image upload.
The module registers the :c:func:`upload_confirm_cb` callback that is used to submit :c:struct:`ble_smp_transfer_event`.
The module registers itself as the final subscriber of the event to track the number of submitted events.
If a :c:struct:`ble_smp_transfer_event` was already submitted but not processed, the module desists from submitting a subsequent event.
After the previously submitted event is processed, the module submits a subsequent event when the :c:func:`upload_confirm_cb` callback is called.

The application user must not perform more than one firmware upgrade at a time.
The modification of the data by multiple application modules can result in a broken image that is going to be rejected by the bootloader.

After building your application with the |smp| enabled, the :file:`dfu_application.zip` archive is generated in the build directory.
It contains all the firmware update files that are necessary to perform DFU.
For more information about the contents of update archive, see :ref:`app_build_output_files`.

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
   :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start
   :end-before: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

.. note::
  If the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option is enabled in the application configuration, the device rejects an update image upload for the invalid slot.
  It is recommended to enable the option if the application uses MCUboot in the direct-xip mode.
