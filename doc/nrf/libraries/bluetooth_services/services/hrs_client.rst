.. _lib_hrs_client_readme:

GATT Heart Rate Service (HRS) Client
####################################

.. contents::
   :local:
   :depth: 2

The GATT Heart Rate Service Client is used to retrieve information about heart rate measurements.

Overview
********

The GATT HRS Client can retrieve information such as the sensor location from a device that provides the Heart Rate Service.
It can also configure the Heart Rate Service on a remote device by writing specific values to the Heart Rate Control Point characteristic.

Configuration
*************

Applications use the :ref:`lib_nrf_bt_scan_readme` for detecting advertising devices that support the Heart Rate Service.
If an advertising device is detected, the application connects to it automatically and starts receiving HRS data.

Once a connection with a remote device providing a Heart Rate Service is established, the client needs service discovery to discover Heart Rate Service handles.
If this succeeds, the handles of the Heart Rate Service must be assigned to a HRS client instance using the :c:func:`bt_hrs_client_handles_assign` function.
Now, the application is ready to operate with the remote Heart Rate Service.

Usage
*****

Retrieve data from the Heart Rate Service or configure its behavior using the following functions:

* :c:func:`bt_hrs_client_measurement_subscribe` - Enable notifications for the Heart Rate Measurement characteristic.

  The notifications are passed to the provided callback function.
  You will receive the current heart rate measurement data.

* :c:func:`bt_hrs_client_sensor_location_read` - Read a heart rate sensor location.

  Check the possible locations in :c:enum:`bt_hrs_client_sensor_location`.

* :c:func:`bt_hrs_client_control_point_write` - Configure a remote Heart Rate Service.

  Check the possible values for a Heart Rate Control Point characteristic in :c:enum:`bt_hrs_client_cp_value`.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`central_and_peripheral_hrs`
* :ref:`bluetooth_central_hr_coded`

Additional information
**********************

Do not use any of the values in the :c:struct:`bt_hrs_client` object structure directly.
All values that should be accessed have accessory functions.
The structure is fully defined because the application must be able to allocate memory for it.

Dependencies
************

* :ref:`gatt_dm_readme`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/conn.h`

API documentation
*****************

| Header file: :file:`include/bluetooth/services/hrs_client.h`
| Source file: :file:`subsys/bluetooth/services/hrs_client.c`

.. doxygengroup:: bt_hrs_client
   :project: nrf
   :members:
