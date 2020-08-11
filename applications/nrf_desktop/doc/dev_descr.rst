.. _nrf_desktop_dev_descr:

Device description module
#########################

The device description module defines custom GATT Service, which contains information about whether the peripheral supports the Low Latency Packet Mode (LLPM).
Only the nrfxlib Link Layer (:option:`CONFIG_BT_LL_SOFTDEVICE`) supports the LLPM extension.
The Service is mandatory for all nRF Desktop peripherals that connect to the nRF Desktop centrals.

Module events
*************

The ``dev_descr`` module does not submit any event nor subscribe for any event.

Configuration
*************

The module is enabled by default for all Bluetooth Peripheral devices.

The :file:`dev_descr.h` file contains the UUIDs used by the custom GATT Service.
The file is located in the :file:`configuration/common` directory.
The UUIDs must be the same for all peripheral devices to ensure that the central is able to read information about the LLPM support.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define the custom GATT Service.
More detailed information regarding GATT are available in Zephyr's :ref:`zephyr:bt_gatt` documentation.
