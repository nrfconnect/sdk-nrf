.. _dev_descr:

Device Description
##################

The ``dev_descr`` module defines custom GATT Service, which contains information if the Peripheral supports LLPM.
Only the nrfxlib Link Layer (:option:`CONFIG_BT_LL_NRFXLIB`) supports the LLPM.
The Service is mandatory to all the nRF52 Desktop Peripherals, which connect to the nRF52 Desktop Central.

Module Events
*************

The module does not submit any event nor subscribe for any event.

Configuration
*************

The module is enabled by default for all the Bluetooth Peripheral devices (when :option:`CONFIG_BT_PERIPHERAL` is set).

``dev_descr.h`` file contains the UUIDs used by the custom GATT Service.
The file is located in the ``nrf_desktop/configuration/commmon`` directory.
The UUIDs must be the same for all the nRF52 Destkop devices to ensure that the nRF52 Desktop Central will be able to properly read information regarding LLPM support from the Peripherals.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define the custom GATT Service.
More detailed information regarding GATT are available in Zephyr documentation: :ref:`zephyr:bt_gatt`.
