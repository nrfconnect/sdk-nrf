.. _lib_ntn:

NTN
###

.. contents::
   :local:
   :depth: 2

The NTN library provides helper functionality for Non-Terrestrial Network (NTN) operations.

.. note::
   This library is only supported by the ``mfw_nrf9151-ntn`` modem firmware.

Overview
********

The NTN library currently provides location update functionality for NTN operations.
The library monitors location update requests from the modem and provides a method for supplying location information.

The library uses the ``AT%LOCATION`` AT command to subscribe to location update requests from the modem and to provide location updates.

Configuration
*************

To enable the NTN library in your project, set the :kconfig:option:`CONFIG_NTN` Kconfig option.

The library has the following configuration options:

* :kconfig:option:`CONFIG_NTN_LOCATION_REQUEST_ADVANCE` - Sets how many seconds in advance the :c:enumerator:`NTN_EVT_LOCATION_REQUEST` event is sent to request location when modem needs location at a specific time.
  The value should be long enough so that the application has enough time to update the current location before modem needs it.
  This should include for example time to start GNSS and get a valid fix.
* :kconfig:option:`CONFIG_NTN_WORKQUEUE_STACK_SIZE` - Stack size for the NTN library work queue.
* :kconfig:option:`CONFIG_NTN_WORKQUEUE_PRIORITY` - Priority for the NTN library work queue.

Usage
*****

The application can register an event handler using the :c:func:`ntn_register_handler` function to receive notifications when the modem requires location updates.
The :c:enumerator:`NTN_EVT_LOCATION_REQUEST` event is sent to notify the application when location updates are requested and the accuracy requested by the modem.

When location updates are requested, the application must call the :c:func:`ntn_location_set` function with the current location.
The location can be acquired for example from an external GNSS receiver.
The required interval for calling the function depends on the maximum speed of the device.
The accuracy of the location should remain within the requested accuracy at all times.
For example, if the requested accuracy is 200 meters and the device can move at a maximum of 120 km/h (33.3 m/s), the location should be updated at least every six seconds.

Stationary or semi-stationary devices may also use the internal GNSS receiver to acquire the location.
When the device is stationary, the location can be updated less frequently.
Internal GNSS is not supported when NTN NB-IoT is enabled in the system mode.
To use internal GNSS to get the location, GNSS must be enabled and NTN NB-IoT disabled in the system mode.
After GNSS has been stopped, you can enable NTN NB-IoT in the system mode.

The validity time parameter in the :c:func:`ntn_location_set` function call must be longer than the time between subsequent location updates.
The location can be invalidated using the :c:func:`ntn_location_invalidate` function.

When the modem requests location updates at a specific time in the future, the library sends the :c:enumerator:`NTN_EVT_LOCATION_REQUEST` event before the requested time.
You can configure how much in advance the event is sent using the :kconfig:option:`CONFIG_NTN_LOCATION_REQUEST_ADVANCE` Kconfig option.
It allows the application enough time to start GNSS and get a valid fix before the modem needs the location.

Samples using the library
**************************

The following |NCS| samples use this library:

* :ref:`modem_shell_application` - The modem shell sample includes NTN commands for testing the NTN library functionality.

Limitations
***********

* The library requires modem firmware with NTN support (``mfw_nrf9151-ntn``).
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
