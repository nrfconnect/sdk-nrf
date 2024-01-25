.. _modem_battery_readme:

Modem battery
#############

.. contents::
   :local:
   :depth: 2

The modem battery library is used to get battery voltage information or notifications from the modem.

The library issues AT commands to perform the following actions:

* Retrieve the battery voltage measured by the modem.
* Set the battery voltage low level for the modem using the ``%XVBATLOWLVL`` command.
  See the `Battery voltage low level %XVBATLOWLVL`_  section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 battery voltage low level %XVBATLOWLVL_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.
* Subscribe to or unsubscribe from unsolicited notifications on modem battery voltage low level.
* Configure the power-off warnings from the modem (possible with modem firmware v1.3.1 and higher).

Configuration
*************

You can enable this library using the Kconfig option :kconfig:option:`CONFIG_MODEM_BATTERY`.
When enabled, the modem battery library automatically subscribes to unsolicited notifications on modem battery voltage low level and configures the power-off warnings from the modem.

Usage
*****

The library can be used for the following purposes:

* Retrieving the modem battery voltage
* Setting the modem battery low level
* Configuring power-off warnings

Retrieving the modem battery voltage
====================================

To retrieve the battery voltage measured (in mV) by the modem, call the :c:func:`modem_battery_voltage_get` function by passing a pointer to an integer variable for the battery voltage.
The :c:func:`modem_battery_voltage_get` function sends the Nordic-proprietary ``%XVBAT`` command that reads battery voltage.
When the modem is active (either involved in LTE communication or acts as a GNSS receiver), the ``%XVBAT`` command returns the latest voltage measured automatically during modem wakeup or reception.
The voltage measured during transmission is not reported.
During modem inactivity, the modem measures battery voltage when the ``%XVBAT`` command is received.

Setting the modem battery low level
===================================

The modem battery low level is a voltage threshold that can be set in the modem to get notified when the battery voltage drops below the set value.
Battery low level notifications are received within a time interval of 60 seconds when the battery voltage has dropped below the threshold while maintaining an LTE connection.

Unsolicited notifications of battery voltage low level are automatically enabled when the modem battery library is enabled using the :kconfig:option:`CONFIG_MODEM_BATTERY` Kconfig option.

An application can follow ``+CSCON`` notifications and expect low battery level notification only after ``+CSCON: 1`` is received.
The factory default battery voltage low level is 3300 mV and the default range is 3100 to 3500 mV.
To update this value, call the :c:func:`modem_battery_low_level_set` function by passing an integer for the battery voltage low level (measured in mV).

To start receiving notifications of battery voltage low level from the modem, first set a handler by calling the :c:func:`modem_battery_low_level_handler_set` function before subscribing.
An integer value for the battery voltage (measured in mV) reported by the notification is passed to the handler.
To manually subscribe to and unsubscribe from battery voltage low level notifications, call the :c:func:`modem_battery_low_level_enable` and the :c:func:`modem_battery_low_level_disable` functions, respectively.

Configuring power-off warnings
==============================

The power-off warning is received as an unsolicited AT notification when the modem battery voltage drops below a certain threshold.
This threshold is not the same as the battery voltage low level threshold.
When the power-off warning is sent, the modem sets itself to offline mode and sends an unsolicited notification.
Active power-off warning blocks the storing to NVM.

The power-off warnings are automatically configured when the modem battery library is enabled using :kconfig:option:`CONFIG_MODEM_BATTERY`.

To start receiving power-off warnings from the modem, first set a handler by calling the :c:func:`modem_battery_pofwarn_handler_set` function before configuring.

Configuring and enabling the modem battery power-off warning is only supported by modem firmware v1.3.1 and higher.
The application is responsible for detecting a possible increase in the battery voltage level and for restarting the LTE protocol activities.
This can be detected by calling the :c:func:`modem_battery_voltage_get` function.
If the level is acceptable again (value greater than 3000 mV), the application can proceed with initialization of the modem.
The factory default is 3000 mV.
To configure this value, call the :c:func:`modem_battery_pofwarn_enable` function by passing an enum corresponding to a voltage value.
The expected values are ``30`` (for 3000 mV), ``31`` (for 3100 mV), ``32`` (for 3200 mV), ``33`` (for 3300 mV) and can be found in the :c:enum:`pofwarn_level` type as :c:enumerator:`POFWARN_3000`, :c:enumerator:`POFWARN_3100`, :c:enumerator:`POFWARN_3200` and :c:enumerator:`POFWARN_3300`, respectively.

To manually configure and disable the power-off warnings, call the :c:func:`modem_battery_pofwarn_enable` and the :c:func:`modem_battery_pofwarn_disable` functions, respectively.

Dependencies
============

The modem battery library uses the :ref:`at_monitor_readme` library.

API documentation
*****************

| Header file: :file:`include/modem/modem_battery.h`
| Source files: :file:`lib/modem_battery/`

.. doxygengroup:: modem_battery
   :project: nrf
   :members:
