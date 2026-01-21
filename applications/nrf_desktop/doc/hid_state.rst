.. _nrf_desktop_hid_state:

HID state module
################

.. contents::
   :local:
   :depth: 2

The |hid_state| is the center of an application acting as a HID peripheral.
It is responsible for the following operations:

* Tracking the state of HID subscribers and HID input report subscriptions.
  The module can simultaneously handle HID input report subscriptions of multiple HID subscribers.
  The module provides HID input reports only to subscribers with the highest priority (*active subscribers*).
* Providing HID input reports to the active HID subscribers.
  The module relies on HID report providers to aggregate the user input, form HID input reports, and submit a :c:struct:`hid_report_event`.
  The HID input reports can be formatted according to either HID report protocol or HID boot protocol.
* Handling HID output reports.
  The module handles only the HID keyboard LED output report.
  The module sends a :c:struct:`led_event` to update state of the keyboard LEDs.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_state_start
    :end-before: table_hid_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To enable the |hid_state|, use the :option:`CONFIG_DESKTOP_HID_STATE_ENABLE` Kconfig option that is implied by the :option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL` option.
Make sure to configure the peripheral type and the set of supported HID input reports and HID boot interface.
For details related to HID configuration in the nRF Desktop, see the :ref:`nrf_desktop_hid_configuration` documentation.

Number of supported HID subscribers
===================================

A HID transport (for example, :ref:`nrf_desktop_hids` or :ref:`nrf_desktop_usb_state`) is a module that forwards HID reports to a HID host and forwards HID input report subscriptions of the HID host.
In most cases, a HID transport registers a single HID subscriber that handles all HID input reports.
If your application configuration supports more than one HID subscriber, you must align the maximum number of HID subscribers that can be handled simultaneously (:option:`CONFIG_DESKTOP_HID_STATE_SUBSCRIBER_COUNT`).
For example, to use a configuration that allows to simultaneously subscribe to HID input reports from HID over GATT (Bluetooth LE) and a single USB HID instance, set the value of this Kconfig option to ``2``.

Selective HID input report subscription
---------------------------------------

In some cases, a single HID transport can register multiple HID subscribers.
Every HID subscriber handles a subset of HID input reports.

For example, an nRF Desktop peripheral might use the USB selective HID report subscription feature to split HID input reports among multiple HID-class USB instances (every HID-class USB instance handles a predefined subset of HID input report IDs).
For more details regarding the feature, see the :ref:`nrf_desktop_usb_state_hid_class_instance` documentation section of the USB state module.

Using selective HID input report subscription requires increasing the value of the :option:`CONFIG_DESKTOP_HID_STATE_SUBSCRIBER_COUNT` Kconfig option.
For example, if a configuration allows simultaneously subscribing to HID input reports from HID over GATT (Bluetooth LE) and two USB HID instances, increase the value of the Kconfig option to ``3``.

HID subscriber priority
-----------------------

If multiple HID subscribers are simultaneously connected, the |hid_state| selects the ones with the highest priority as the active subscribers.
The |hid_state| provides HID input reports only to the active subscribers.
The |hid_state| displays the HID keyboard LED state associated with the active subscriber of the HID keyboard input report.
HID subscribers with the same priority cannot simultaneously subscribe to the same HID input report.

If a HID transport uses a selective HID input report subscription, all subscribers registered by the transport must share the same priority.
Otherwise, subscribers with lower priority would not receive HID input reports from the HID state.

.. note::
   By default, a subscriber that is associated with USB has priority over a subscriber associated with Bluetooth LE.
   If a HID host connects through the USB while another HID host is connected over the Bluetooth LE, the HID reports will be routed to the USB.

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

You must define all of the mentioned data in this configuration file, and specify its location with the :option:`CONFIG_DESKTOP_HID_STATE_HID_KEYBOARD_LEDS_DEF_PATH` Kconfig option.

.. note::
   The configuration file should be included only by the configured module.
   Do not include the configuration file in other source files.

HID report providers
====================

The |hid_state| relies on the HID report providers to collect user input, form HID input reports, and submit a :c:struct:`hid_report_event`.
The module selects the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_EVENT` Kconfig option to enable the HID report provider event and default HID report providers for all HID input reports enabled in the configuration.
The HID providers for mouse and keyboard input reports also handle the respective HID boot input reports if the boot report support is enabled in the configuration.

.. note::
   You can provide an alternative implementation of a HID report provider to generate a HID report in a custom way.
   You can also add a new HID report provider to introduce support for another HID input report.
   See the :ref:`nrf_desktop_hid_state_providing_hid_input_reports` section for implementation details related to HID report providers integration.

Implementation details
**********************

This section describes implementation details related to responsibilities of the |hid_state|.

Tracking state of HID subscribers
=================================

A HID transport reports the state of a HID subscriber using the :c:struct:`hid_report_subscriber_event`.
When the connection to the HID host is indicated by this event, the |hid_state| will create an associated subscriber.
The |hid_state| tracks the state of the HID subscribers.

As part of the :c:struct:`hid_report_subscriber_event`, the subscriber provides the following parameters:

* Subscriber priority - The |hid_state| provides HID input reports only to subscribers with the highest priority (active subscribers).
* Pipeline size - The |hid_state| forwards this information to the HID report providers.
  The information can be used, for example, to synchronize sensor sampling with sending the HID input reports to the HID host.
  See the :ref:`nrf_desktop_hid_mouse_report_handling` section for information how the pipeline size is used for HID mouse reports.
* Maximum number of processed HID input reports - The |hid_state| limits the number of HID input reports processed by a HID subscriber at a time by delaying providing the subsequent HID input report until the previous report is sent to a HID host.

Tracking state of HID report subscriptions
------------------------------------------

For each subscriber, the |hid_state| tracks the state of HID input report subscriptions.
The HID input reports are only provided after one of the active subscribers enables the subscription.
The subscriber updates its HID report subscriptions using a :c:struct:`hid_report_subscription_event`.

The HID report subscriptions are tracked in the subscriber's structure :c:struct:`subscriber`.
This structure's member ``state`` is an array of :c:struct:`report_state` structures.
Each element corresponds to one HID input report.

The :c:struct:`report_state` structure serves the following purposes:

* Tracks the state of the report subscription.
* Contains the link connecting the object to the right :c:struct:`provider` structure which contains the HID report provider info such as report ID and API (:c:struct:`hid_report_provider_api`).
* Tracks the number of reports with a given ID in flight.

.. _nrf_desktop_hid_state_providing_hid_input_reports:

Providing HID input reports
===========================

The |hid_state| relies on the HID providers to collect user input, form HID input reports, and submit a :c:struct:`hid_report_event`.
Every HID input report ID is handled by a dedicated HID report provider API (:c:struct:`hid_report_provider_api`).

HID report provider event
-------------------------

The :c:struct:`hid_report_provider_event` is used to establish two-way callbacks between the |hid_state| and the HID report providers.
The event allows to exchange the API structures between the |hid_state| and HID report providers (:c:struct:`hid_report_provider_api` and :c:struct:`hid_state_api`).
The API structures allow for direct function calls between the modules.

The |hid_state| requests the HID report providers to generate HID reports and notifies the providers about the connection state changes and report sent occurrences.
The HID report providers can notify the |hid_state| when new data is available (on user input) to trigger generating a HID input report.

On a |hid_state|'s request, a HID report provider submits a :c:struct:`hid_report_event` to provide a HID input report to the active HID subscriber.
The :c:struct:`hid_report_sent_event` is submitted by the HID transport related to the subscriber to confirm that the HID report was sent to the HID host.
The |hid_state| relies on this event to track the number of HID reports in flight and notify the providers.

See the :c:struct:`hid_report_provider_event` event documentation page for detailed information regarding the communication between the |hid_state| and HID report providers.

Default HID providers
---------------------

The following application modules are used as default implementations of HID report providers:

* :ref:`nrf_desktop_hid_provider_mouse`
* :ref:`nrf_desktop_hid_provider_keyboard`
* :ref:`nrf_desktop_hid_provider_system_ctrl`
* :ref:`nrf_desktop_hid_provider_consumer_ctrl`

The respective HID report provider is automatically enabled if support for a given HID input report is enabled in the :ref:`nrf_desktop_hid_configuration`.
See the documentation page of a HID report provider for detailed information about the provider.

Custom HID providers
--------------------

You can implement your own HID report provider as part of the application.
The HID report provider can perform one of the following two actions:

* Handle a HID input report that is already supported by the application instead of a default HID report provider (substitute the default HID report provider).
  Make sure to disable the default HID report provider while implementing the custom provider.
* Support a new HID input report.

HID report map update
~~~~~~~~~~~~~~~~~~~~~

If your HID report provider implementation uses a different HID input report format or you add a new HID input report, you need to align the HID report configuration (including the HID report map).
If the default HID report descriptor is used (:option:`CONFIG_DESKTOP_USE_DEFAULT_REPORT_DESCR`), the configuration is defined by the following files:

* :file:`configuration/common/hid_report_desc.h`
* :file:`configuration/common/hid_report_desc.c`

.. note::
   nRF Desktop dongles share a common HID report format with the nRF Desktop peripherals.
   The aligned HID report configuration is required for the dongle to forward HID input reports from the peripherals.

HID transport update
~~~~~~~~~~~~~~~~~~~~

If you add a new HID input report, you might also need to update the modules that act as HID transports (for example :ref:`nrf_desktop_hids` or :ref:`nrf_desktop_usb_state`).
This is needed to fulfill the following requirements:

* Proper configuration of the module and libraries used by the module.
* Support for the newly added HID input report.

HID output reports
==================

When the |hid_state| receives a :c:struct:`hid_report_event` that contains a HID output report, it updates the stored information about the state of the HID output report of the appropriate subscriber.

By default, nRF Desktop supports only HID keyboard LED output report.
The nRF Desktop peripheral displays the state of the keyboard LEDs that was specified by the active HID subscriber of a HID keyboard input report.
When the subscriber changes or updates the state of the keyboard LEDs, the |hid_state| sends a :c:struct:`leds_event` to update the state of the hardware LEDs.
