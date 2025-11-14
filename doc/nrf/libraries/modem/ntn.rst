.. _lib_ntn:

NTN
###

.. contents::
   :local:
   :depth: 2

The NTN (Non-Terrestrial Network) library provides helper functionality for NTN operations.

.. note::
   This library is only supported by the following modem firmware:
   - mfw_nrf9151-ntn

Overview
********

The NTN library currently provides location update functionality for NTN operations.
The library monitors location update requests from the modem and provides two methods for supplying location information, periodic updates from external GNSS and direct location setting.

The library uses the ``AT%LOCATION`` AT command to subscribe to location update requests from the modem and to provide location updates.

Configuration
*************

You can enable the NTN library in your project by setting the following Kconfig option:

* :kconfig:option:`CONFIG_NTN` - Enable the NTN library.

The library has the following configuration options:

* :kconfig:option:`CONFIG_NTN_MODEM_LOCATION_REQ_MARGIN` - Margin in seconds for requesting location when modem needs location at a specific time.
  The margin should be long enough so that the application has enough time to update the current location before the modem needs it.
  This should include, for example, time to start GNSS and get a valid fix.

* :kconfig:option:`CONFIG_NTN_WORKQUEUE_STACK_SIZE` - Stack size for the NTN library work queue.

* :kconfig:option:`CONFIG_NTN_WORKQUEUE_PRIORITY` - Priority for the NTN library work queue.

Usage
*****

Periodic updates from an external GNSS
======================================

The application can register an event handler using :c:func:`ntn_register_handler` to receive notifications when the modem requires location updates.
The :c:enumerator:`NTN_EVT_MODEM_LOCATION_UPDATE` event is sent to notify the application when location updates are requested.

When location updates are requested, the application must call :c:func:`ntn_modem_location_update` periodically with the current location from an external GNSS receiver.
The required interval for calling the function depends on the maximum speed of the device.
The accuracy of the location should remain within 200 meters at all times.
If the device can move at a maximum of 100 km/h (27.8 m/s), the location should be updated at least every 6 seconds, preferably every 5 seconds.
The function can also be called once a second, because location will only be updated to the modem by the library when it is required to maintain the requested accuracy.
This reduces the number of AT commands sent to the modem.

The library calculates the distance between the current location and the last location sent to the modem.
It only sends location updates to the modem when the distance exceeds the requested accuracy threshold (minus a safety margin).
The library also calculates the validity time for location updates based on the requested accuracy and the current speed of the device.

When the modem requests location updates at a specific time in the future, the library sends the :c:enumerator:`NTN_EVT_MODEM_LOCATION_UPDATE` event with a margin before the requested time.
This margin is configurable using :kconfig:option:`CONFIG_NTN_MODEM_LOCATION_REQ_MARGIN` and allows the application enough time to start GNSS and get a valid fix before the modem needs the location.

Direct location setting
=======================

For stationary or semi-stationary devices, the application can use :c:func:`ntn_modem_location_set` to set the location directly to the modem with a specified validity time.
The location can be invalidated using :c:func:`ntn_modem_location_invalidate` when it is no longer valid.

Samples using the library
**************************

The following |NCS| samples use this library:

* :ref:`modem_shell_application` - The modem shell sample includes NTN commands for testing the NTN library functionality.

Limitations
***********

* The library requires modem firmware with NTN support (mfw_nrf9151-ntn).
* Only one event handler can be registered at a time.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`nrf_modem_lib_readme`
* :ref:`at_parser_readme`
* :ref:`at_monitor_readme`

API documentation
*****************

| Header file: :file:`include/modem/ntn.h`
| Source files: :file:`lib/ntn/`

.. doxygengroup:: ntn
