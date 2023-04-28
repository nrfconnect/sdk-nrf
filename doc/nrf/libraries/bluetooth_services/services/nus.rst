.. _nus_service_readme:

Nordic UART Service (NUS)
#########################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Low Energy (LE) GATT Nordic UART Service is a custom service that receives and writes data and serves as a bridge to the UART interface.

The NUS Service is used in the :ref:`peripheral_uart` sample.
It is also included with the Thingy:91 :ref:`connectivity_bridge`, but is disabled by default, and with the :ref:`Matter door lock sample <matter_lock_sample>`, as an optional feature that extends the Bluetooth LE connectivity.

Service UUID
************

The 128-bit vendor-specific service UUID is ``6E400001-B5A3-F393-E0A9-E50E24DCCA9E`` (16-bit offset: ``0x0001``).

Characteristics
***************

This service has two characteristics.

RX Characteristic (``6E400002-B5A3-F393-E0A9-E50E24DCCA9E``)
============================================================

Write or Write Without Response
   Write data to the RX Characteristic to send it to the UART interface.

TX Characteristic (``6E400003-B5A3-F393-E0A9-E50E24DCCA9E``)
============================================================

Notify
   Enable notifications for the TX Characteristic to receive data from the application.
   The application transmits all data that is received over UART as notifications.


API documentation
*****************

| Header file: :file:`include/bluetooth/services/nus.h`
| Source file: :file:`subsys/bluetooth/services/nus.c`

.. doxygengroup:: bt_nus
   :project: nrf
   :members:
