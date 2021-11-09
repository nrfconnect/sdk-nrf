.. _lib_hrs_client_readme:

GATT Heart Rate Service (HRS) Client
####################################

.. contents::
   :local:
   :depth: 2

You can use the Heart Rate Service Client to retrieve information about heart rate measurements, such as the sensor location from a device that provides a Heart Rate Service.
The client can also configure the Heart Rate Service on a remote device by writing specific values to the Heart Rate Control Point characteristic.

Library files
*************

Files used by Heart Rate Service Client.

| Header file: :file:`include/bluetooth/services/hrs_client.h`
| Source file: :file:`subsys/bluetooth/services/hrs_client.c`

Usage
*****

.. note::
   Do not use any of the values in the :c:struct:`bt_hrs_client` object structure directly.
   All values that should be accessed have accessory functions.
   The structure is fully defined because the application must be able to allocate memory for it.

Once a connection with a remote device providing a Heart Rate Service is established, the client needs service discovery to discover Heart Rate Service handles.
If this succeeds, the handles of the Heart Rate Service must be assigned to a HRS client instance using the :c:func:`bt_hrs_client_handles_assign` function.

The application can now operate with the remote Heart Rate Service.

You can now retrieve data from a Heart Rate service or configure its behavior using the following operations:

Notifications
  Use the :c:func:`bt_hrs_client_measurement_subscribe` function to enable notifications for the Heart Rate Measurement characteristic.
  The notifications are passed to the provided callback function.
  You will receive the current heart rate measurement data.

Reading the sensor body location
  Use the :c:func:`bt_hrs_client_sensor_location_read` function to read a heart rate sensor location.
  Check the possible locations in :c:enum:`bt_hrs_client_sensor_location`.

Writing to the control point
  To configure a remote Heart Rate Service, use the :c:func:`bt_hrs_client_control_point_write` function.
  Check the possible values for a Heart Rate Control Point characteristic in :c:enum:`bt_hrs_client_cp_value`.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`central_and_peripheral_hrs`
* :ref:`bluetooth_central_hr_coded`

Dependencies
************

* :ref:`gatt_dm_readme`
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/conn.h``

API documentation
*****************

| Header file: :file:`include/bluetooth/services/hrs_client.h`
| Source file: :file:`subsys/bluetooth/services/hrs_client.c`

.. doxygengroup:: bt_hrs_client
   :project: nrf
   :members:
