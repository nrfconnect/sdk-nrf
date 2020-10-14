.. _nrf_desktop_dev_descr:

Device description module
#########################

.. contents::
   :local:
   :depth: 2

The device description module defines custom GATT Service, which contains:

* Information about whether the peripheral supports the Low Latency Packet Mode (LLPM)
* Hardware ID (HW ID) of the peripheral

In order to support the LLPM, the peripheral must use the SoftDevice Link Layer (:option:`CONFIG_BT_LL_SOFTDEVICE`) and :option:`CONFIG_DESKTOP_BLE_USE_LLPM` Kconfig option must be enabled.

The Service is mandatory for all nRF Desktop peripherals that connect to the nRF Desktop centrals.

Module events
*************

The ``dev_descr`` module does not submit any event nor subscribe for any event.

Configuration
*************

The module uses Zephyr's :ref:`zephyr:hwinfo_api` to obtain the hardware ID.
Enable the required driver using :option:`CONFIG_HWINFO`.

The module is enabled by default for all nRF Desktop peripheral devices.

The :file:`dev_descr.h` file contains the UUIDs used by the custom GATT Service.
The file is located in the :file:`configuration/common` directory.
The UUIDs must be the same for all devices to ensure that the central is able to read the provided information.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define the custom GATT Service.
More detailed information regarding GATT are available in Zephyr's :ref:`zephyr:bt_gatt` documentation.
