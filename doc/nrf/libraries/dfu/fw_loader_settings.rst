.. _lib_fw_loader_settings:

Firmware loader settings
########################

.. contents::
   :local:
   :depth: 2

.. note::
   The support for this feature is currently experimental.

This module is intended for Bluetooth LE applications that use the Bluetooth LE MCUmgr firmware
loader.
It lets the main application set the advertising name used by the firmware loader image through Zephyr
:ref:`Settings <zephyr:settings_api>` storage.
The name is stored under the ``fw_loader/adv_name`` key and can be updated remotely by the client
using the MCUmgr settings management group.
By assigning a unique name before reset, the DFU client can reconnect to the same device based on
that name once it has rebooted into the firmware loader.

Configuration
*************

For sysbuild projects with a Bluetooth LE firmware loader image, enable
:kconfig:option:`CONFIG_FW_LOADER_SETTINGS_ADV_NAME` in the main application and firmware loader image
Kconfig fragments.
The firmware loader image must also enable a Settings backend that can access the shared storage.
Remote MCUmgr settings access is restricted to ``fw_loader/adv_name`` by default when
:kconfig:option:`CONFIG_FW_LOADER_SETTINGS_ADV_NAME_ACCESS_PROTECT` is enabled.

See the :ref:`single_slot_sample` sample (``FILE_SUFFIX=ble_enter`` build) for a complete
configuration example.

API documentation
*****************

| Header file: :file:`include/dfu/fw_loader_settings.h`
| Source files: :file:`subsys/dfu/fw_loader_settings/src/`

.. doxygengroup:: fw_loader_settings
