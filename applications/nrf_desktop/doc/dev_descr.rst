.. _nrf_desktop_dev_descr:

Device description module
#########################

.. contents::
   :local:
   :depth: 2

The device description module defines custom GATT Service, which contains:

* Information about whether the peripheral supports the Low Latency Packet Mode (LLPM)
* Hardware ID (HW ID) of the peripheral

To support the LLPM, the peripheral must use the SoftDevice Link Layer.
This means that you must enable both the :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE` and the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` Kconfig options.

The Service is mandatory for all nRF Desktop peripherals that connect to the nRF Desktop centrals.

Module events
*************

The ``dev_descr`` module does not submit any event nor subscribe for any event.

Configuration
*************

The module requires the basic Bluetooth configuration, as described in the :ref:`nrf_desktop_bluetooth_guide` documentation.
Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` options are enabled.
The device description application module is enabled by the :ref:`CONFIG_DESKTOP_DEV_DESCR_ENABLE <config_desktop_app_options>` option, which is implied by :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` together with other features used by a Bluetooth LE HID peripheral device.

The module selects the :ref:`CONFIG_DESKTOP_HWID <config_desktop_app_options>` option to make sure that the nRF Desktop Hardware ID utility is enabled.
The utility uses Zephyr's :ref:`zephyr:hwinfo_api` to obtain the hardware ID and selects the :kconfig:option:`CONFIG_HWINFO` Kconfig option to automatically enable the required driver.

The :file:`dev_descr.h` file contains the UUIDs used by the custom GATT Service.
The file is located in the :file:`configuration/common` directory.
The UUIDs must be the same for all devices to ensure that the central is able to read the provided information.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define the custom GATT Service.
More detailed information regarding GATT is available in Zephyr's :ref:`zephyr:bt_gatt` documentation.
