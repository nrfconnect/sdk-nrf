.. _nrf_desktop_hid_state:

HID state module
################

.. contents::
   :local:
   :depth: 2

The |hid_state| is required for communicating with the HID report providers to generate reports from input data.
It is responsible for the following operations:

* Tracking state of the HID report subscriptions.
* Notifying the HID report providers to form the HID reports in either report or boot protocol.
* Notifying the HID report providers about state changes of the HID report subscriber connection.
* Sending :c:struct:`led_event` based on the HID keyboard LED output reports.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_state_start
    :end-before: table_hid_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To enable the |hid_state|, use the :ref:`CONFIG_DESKTOP_HID_STATE_ENABLE <config_desktop_app_options>` Kconfig option that is implied by the :ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>` option.
An nRF Desktop peripheral uses the |hid_state| and HID report providers to generate HID reports based on the user input.
For details related to HID configuration in the nRF Desktop, see the :ref:`nrf_desktop_hid_configuration` documentation.

To send boot reports, enable the respective Kconfig option:

* :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD <config_desktop_app_options>` - This option enables sending keyboard boot reports.
* :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>` - This option enables sending mouse boot reports.

Number of supported HID data subscribers
========================================

If your application configuration supports more than one HID data subscriber, you must align the maximum number of HID data subscribers that can be supported simultaneously (:ref:`CONFIG_DESKTOP_HID_STATE_SUBSCRIBER_COUNT <config_desktop_app_options>`).
For example, to use a configuration that allows to simultaneously subscribe for HID reports from HID over GATT (BLE) and a single USB HID instance, set the value of this Kconfig option to ``2``.
See the `Tracking state of transports`_ section for more details about HID subscribers.

HID keyboard LEDs
=================

You must define which hardware LEDs are used to display state of the HID keyboard LEDs report and LED effects that should be used to display the state.
See documentation of :ref:`caf_leds` for details about LED effects.

You must create a configuration file with the following data:

* ``keyboard_led_on`` - LED effect defined to represent LED turned on by the host.
* ``keyboard_led_off`` - LED effect defined to represent LED turned off by the host.
* ``keyboard_led_map`` - IDs of the hardware LEDs used to represent state of the HID keyboard LED.

For example, the file contents should look like follows:

.. code-block:: c

	#include "hid_keyboard_leds.h"

	static const struct led_effect keyboard_led_on = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255));
	static const struct led_effect keyboard_led_off = LED_EFFECT_LED_OFF();

	static const uint8_t keyboard_led_map[] = {
		[HID_KEYBOARD_LEDS_NUM_LOCK] = 2,
		[HID_KEYBOARD_LEDS_CAPS_LOCK] = 3,
		[HID_KEYBOARD_LEDS_SCROLL_LOCK] = LED_UNAVAILABLE,
		[HID_KEYBOARD_LEDS_COMPOSE] = LED_UNAVAILABLE,
		[HID_KEYBOARD_LEDS_KANA] = LED_UNAVAILABLE,
	};

You must define all of the mentioned data in this configuration file, and specify its location with the :ref:`CONFIG_DESKTOP_HID_STATE_HID_KEYBOARD_LEDS_DEF_PATH <config_desktop_app_options>` Kconfig option.

.. note::
   The configuration file should be included only by the configured module.
   Do not include the configuration file in other source files.

HID report providers
====================

The module selects the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_EVENT <config_desktop_app_options>` Kconfig option to enable :c:struct:`hid_report_provider_event` event support.
The events are used to establish two-way callbacks between the |hid_state| and the HID report providers.
The |hid_state| can request the HID report providers to generate HID reports and notify the providers about the connection state changes and report sent occurrences.
The HID report providers are responsible for generating HID reports when requested by the |hid_state|.
The HID report providers can also notify the |hid_state| when new data is available.

Implementation details
**********************

The |hid_state| in association with the HID report providers provides a routing mechanism between sources of input data and transport modules.
This can be associated with:

* Receiving input events from :ref:`caf_buttons`, :ref:`nrf_desktop_wheel`, and :ref:`nrf_desktop_motion`.
* Sending out HID reports to HID transports, for example, :ref:`nrf_desktop_hids` and :ref:`nrf_desktop_usb_state`.

Apart from the routing mechanism, the module is also responsible for `Handling HID keyboard LED state`_.

Tracking state of transports
============================

The |hid_state| tracks the state of modules that forward the HID data to a HID host (HID transports) by listening to :c:struct:`hid_report_subscriber_event`.
The |hid_state| refers collectively to all transports as *subscribers*.
When the connection to the host is indicated by this event, the |hid_state| will create a subscriber associated with the transport.
Each subscriber reports its priority as part of the :c:struct:`hid_report_subscriber_event`.
The subscriber priority must be unique, that mean two or more subscribers cannot share the same priority value.

By default, the subscriber that is associated with USB has priority over any Bluetooth LE peer subscriber.
As a result, when the host connected through the USB subscribes for a HID report, the HID report will be routed to the USB.

Tracking state of HID report notifications
==========================================

For each subscriber, the |hid_state| tracks the state of notifications for each of the available HID reports.
These are tracked in the subscriber's structure :c:struct:`subscriber`.
This structure's member ``state`` is an array of :c:struct:`report_state` structures.
Each element corresponds to one available HID report.

The subscriber connects to the HID reports by submitting :c:struct:`hid_report_subscription_event`.
Depending on the connection method, this event can be submitted:

* For Bluetooth, when the notification is enabled for a given HID report.
* For USB, when the device is connected to USB.

The :c:struct:`report_state` structure serves the following purposes:

* Tracks the state of the connection.
* Contains the link connecting the object to the right :c:struct:`provider` structure which contains the HID report provider info such as report ID and API (:c:struct:`hid_report_provider_api`).
* Tracks the number of reports of the associated type that were sent to the subscriber.

Requesting HID reports
======================

When a HID report is to be sent to the subscriber, the |hid_state| calls the appropriate function from the :c:struct:`hid_report_provider_api` to trigger the HID report provider to generate HID report.

Handling HID keyboard LED state
===============================

When the |hid_state| receives a :c:struct:`hid_report_event` that contains a HID output report, it updates the remembered information about the state of the HID output report of the appropriate subscriber.

By default, nRF Desktop supports only HID keyboard LED output report.
The nRF Desktop peripheral displays the state of the keyboard LEDs that was specified by the HID subscriber that subscribed for keyboard key HID input report.
When the subscriber is changed or it updates the state of the keyboard LEDs, the |hid_state| sends :c:struct:`leds_event` to update the state of the hardware LEDs.
