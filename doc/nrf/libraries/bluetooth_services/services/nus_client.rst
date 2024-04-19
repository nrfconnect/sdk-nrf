.. _nus_client_readme:

Nordic UART Service (NUS) Client
################################

.. contents::
   :local:
   :depth: 2

You can use the Nordic UART Service Client library to interact with a connected peer that is running the GATT server with the :ref:`nus_service_readme`.
The client uses the :ref:`gatt_dm_readme` library to acquire the attribute handles that are necessary to interact with the RX and TX Characteristics.

The NUS Service Client is used in the :ref:`central_uart` sample.


RX Characteristic
*****************

To send data to the RX Characteristic, use the :c:func:`bt_nus_client_send` function of this library.
The sending procedure is asynchronous, so the data to be sent must remain valid until a dedicated callback notifies you that the write request has been completed.

TX Characteristic
*****************

To receive data coming from the TX Characteristic, enable notifications after the service discovery.
After that, you can request to disable notifications again by returning the ``STOP`` value in the callback that is used to handle incoming data.
Another dedicated callback is triggered when your request has been completed, to inform that you have unsubscribed successfully.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/nus_client.h`
| Source file: :file:`subsys/bluetooth/services/nus_client.c`

.. doxygengroup:: bt_nus_client
   :project: nrf
   :members:
