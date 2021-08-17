.. _nus_service_readme:

Nordic UART Service (NUS)
#########################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE GATT Nordic UART Service is a custom service that receives and writes data and serves as a bridge to the UART interface.

The NUS Service is used in the :ref:`peripheral_uart` sample.

Service UUID
************

The 128-bit vendor-specific service UUID is 6E400001-B5A3-F393-E0A9-E50E24DCCA9E  (16-bit offset: 0x0001).

Characteristics
***************

This service has two characteristics.

RX Characteristic (6E400002-B5A3-F393-E0A9-E50E24DCCA9E)
========================================================

Write or Write Without Response
   Write data to the RX Characteristic to send it on to the UART interface.

TX Characteristic (6E400003-B5A3-F393-E0A9-E50E24DCCA9E)
========================================================

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
