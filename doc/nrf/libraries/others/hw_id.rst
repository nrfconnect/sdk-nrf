.. _lib_hw_id:

Hardware ID
###########

.. contents::
   :local:
   :depth: 2

The hardware ID library provides a simple interface to retrieve a unique hardware ID.
You can retrieve the hardware ID by calling the :c:func:`hw_id_get` function.

The :ref:`hw_id_sample` sample uses this library.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_HW_ID_LIBRARY` Kconfig option to y in the project configuration file :file:`prj.conf`.

You can configure one of the following Kconfig options to choose the hardware ID:

* :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_IMEI` - This option specifies the :term:`International Mobile (Station) Equipment Identity (IMEI)` of the modem.
* :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_UUID` - This option specifies the UUID of the modem.
* :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC` - This option specifies the default Bluetooth® Low Energy MAC address.
* :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC` - This option specifies the MAC address of the default network interface.
* :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID` - This option specifies a serial number provided by Zephyr's HW Info API.

API documentation
*****************

| Header file: :file:`include/hw_id.h`
| Source files: :file:`lib/hw_id/`

.. doxygengroup:: hw_id
   :project: nrf
   :members:
