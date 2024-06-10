.. _nrf_desktop_dfu_mcumgr:

Device Firmware Upgrade MCUmgr module
#####################################

.. contents::
   :local:
   :depth: 2

The Device Firmware Upgrade MCUmgr module is responsible for performing a Device Firmware Upgrade (DFU) over Simple Management Protocol (SMP).
The module supports both MCUboot bootloader and SUIT for performing DFU.

If you enable the Bluetooth LE as transport, you can perform DFU using, for example, the `nRF Connect Device Manager`_ application.
See the :ref:`nrf_desktop_image_transfer_over_smp` section for more details.

.. note::
    The Device Firmware Upgrade MCUmgr module implementation is currently marked as experimental because it uses internal MCUmgr API.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_dfu_mcumgr_start
    :end-before: table_dfu_mcumgr_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To enable the module, set the :ref:`CONFIG_DESKTOP_DFU_MCUMGR_ENABLE <config_desktop_app_options>` Kconfig option.

The module selects the following configurations:

* :kconfig:option:`CONFIG_MCUMGR` - This option enables the MCUmgr support, which is required for the DFU process.
  For details, see :ref:`zephyr:mcu_mgr` in the Zephyr documentation.

* :kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` - This option enables the MCUmgr notification hook support, which allows the module to listen for an MCUmgr event.
  For details, see :ref:`zephyr:mcumgr_callbacks` in the Zephyr documentation.

  MCUmgr notifications hooks:

    * :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` - This option enables the MCUmgr SMP command status callbacks, which inform when an SMP command is received or processed.

* MCUmgr dependencies:

  * :kconfig:option:`CONFIG_NET_BUF`
  * :kconfig:option:`CONFIG_ZCBOR`
  * :kconfig:option:`CONFIG_CRC`
  * :kconfig:option:`CONFIG_FLASH_MAP`
  * :kconfig:option:`CONFIG_FLASH`

You must also enable the preferred transport for the MCUmgr's SMP protocol (for example, the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` Kconfig option).
With the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` Kconfig option enabled, the module enables the :kconfig:option:`CONFIG_CAF_BLE_SMP_TRANSFER_EVENTS` event.
The event can be used to lower Bluetooth connection latency during the DFU process.

The DFU module leverages the :ref:`nrf_desktop_dfu_lock` to synchronize non-volatile memory access with other DFU methods (for example, the :ref:`nrf_desktop_dfu`).
Set the :ref:`CONFIG_DESKTOP_DFU_LOCK <config_desktop_app_options>` Kconfig option to enable this feature.
Make sure that the DFU lock utility is enabled if your nRF Desktop application configuration uses multiple DFU transports.

You cannot use this module with the :ref:`caf_ble_smp`.
In other words, you cannot simultaneously enable the :ref:`CONFIG_DESKTOP_DFU_MCUMGR_ENABLE <config_desktop_app_options>` option and the :kconfig:option:`CONFIG_CAF_BLE_SMP` Kconfig option.

You have to choose the backend for the DFU process by enabling one of the following options:

  * :kconfig:option:`CONFIG_DESKTOP_DFU_BACKEND_MCUBOOT` - This option depends on the MCUboot bootloader support for the DFU process.
    For details, see :ref:`mcuboot` in the Zephyr documentation.
  * :kconfig:option:`CONFIG_DESKTOP_DFU_BACKEND_SUIT` - This option depends on the SUIT support for the DFU process.
    For details, see :ref:`ug_nrf54h20_suit_dfu` in the Zephyr documentation.

.. note::
    B0 bootloader is not integrated with MCUmgr.

.. _nrf_desktop_dfu_mcumgr_mcuboot:

MCUmgr configuration using MCUboot backend
==========================================

The MCUboot backend configuration selects the following options:

* MCUmgr groups:

  * :kconfig:option:`CONFIG_MCUMGR_GRP_IMG` - This option enables the MCUmgr image management handlers that are required for the DFU process.
    For details, see :ref:`zephyr:device_mgmt` in the Zephyr documentation.
  * :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_MUTEX`
  * :kconfig:option:`CONFIG_MCUMGR_GRP_OS` - This option enables the MCUmgr OS management handlers that are required for the DFU process.
    For details, see :ref:`zephyr:device_mgmt` in the Zephyr documentation.

* :kconfig:option:`CONFIG_IMG_MANAGER`
* :kconfig:option:`CONFIG_STREAM_FLASH`
* :kconfig:option:`CONFIG_MCUBOOT_BOOTUTIL_LIB`

To use the module with this backend, you must also enable the ``SB_CONFIG_BOOTLOADER_MCUBOOT`` Kconfig option.

By default, the MCUmgr DFU module confirms the image using the :c:func:`boot_write_img_confirmed` function during the system boot.
If the :kconfig:option:`CONFIG_DESKTOP_DFU_MCUMGR_MCUBOOT_DIRECT_XIP` option is enabled, the MCUmgr DFU module assumes that the bootloader simply boots the image with a higher version and does not confirm the newly updated image after a successful boot.
Make sure that :kconfig:option:`CONFIG_DESKTOP_DFU_MCUMGR_MCUBOOT_DIRECT_XIP` Kconfig option is enabled, if devices use the MCUboot bootloader in direct-xip mode without revert.
The option is enabled by default if :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` is enabled.

.. _nrf_desktop_dfu_mcumgr_suit:

MCUmgr configuration using SUIT backend
=======================================

The SUIT backend configuration selects the following options:

* :kconfig:option:`CONFIG_MGMT_SUITFU` - This option enables the SUIT Firmware Update Management that implements MCUmgr image management handlers required for the DFU process.
* :kconfig:option:`CONFIG_MGMT_SUITFU_GRP_SUIT` - This option is needed to upload information about all manifests present on the device.
* :kconfig:option:`CONFIG_SSF_SUIT_SERVICE_ENABLED`
* :kconfig:option:`CONFIG_ZCBOR_CANONICAL`
* :kconfig:option:`CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR`

To use the module with this backend, you must also enable the :kconfig:option:`CONFIG_SUIT` Kconfig option.
See :ref:`ug_nrf54h20_suit_dfu` for more information.

Implementation details
**********************

The module uses MCUmgr's SMP command received callback for the following purposes:

* To periodically submit a :c:struct:`ble_smp_transfer_event` while image upload over Bluetooth LE is in progress.
  The module registers itself as the final subscriber of the event to track the number of submitted events.
  If a :c:struct:`ble_smp_transfer_event` was already submitted, but was not yet processed, the module desists from submitting subsequent events.
* To reject image upload or system reboot request if :ref:`nrf_desktop_dfu_lock` is already taken by another DFU transport.

The DFU MCUmgr implementation uses the :ref:`nrf_desktop_dfu_lock`.
On each DFU attempt, the module attempts to claim ownership over the DFU non-volatile memory using the :ref:`nrf_desktop_dfu_lock` API.
It holds the DFU owner status until the DFU process is completed or timed out.
The module assumes that DFU is timed out if the registered MCUmgr notification hooks are not called for 5 seconds.
If the module is not the current DFU owner, it rejects the DFU commands that either write to the DFU non-volatile memory or reboot the device.
The module also resets the MCUmgr DFU progress once the lock is claimed by a different owner.
