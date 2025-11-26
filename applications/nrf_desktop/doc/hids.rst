.. _nrf_desktop_hids:

HID Service module
##################

.. contents::
   :local:
   :depth: 2

Use the HID Service module to handle the GATT Human Interface Device Service.
The GATT Service is used to exchange HID data with the peer connected over Bluetooth®.
For this reason, it is mandatory for every nRF Desktop peripheral.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hids_start
    :end-before: table_hids_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Complete the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.

Make sure that both :option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL` and :option:`CONFIG_DESKTOP_BT_PERIPHERAL` Kconfig options are enabled.
The HID Service application module is enabled by the :option:`CONFIG_DESKTOP_HIDS_ENABLE` option, which is implied by :option:`CONFIG_DESKTOP_BT_PERIPHERAL` together with other GATT Services that are required for a HID device.

GATT Service configuration
==========================

The :option:`CONFIG_DESKTOP_HIDS_ENABLE` option selects the following Kconfig options:

* The :kconfig:option:`CONFIG_BT_HIDS` option that automatically enables the :ref:`hids_readme`.
* The :kconfig:option:`CONFIG_BT_CONN_CTX` option that automatically enables the :ref:`bt_conn_ctx_readme`, which is required by the |GATT_HID|.

The nRF Desktop application modifies the default Kconfig option values, defined by the :ref:`hids_readme`, to tailor the default configuration to application needs.
The configuration is tailored for either nRF Desktop mouse (:option:`CONFIG_DESKTOP_PERIPHERAL_TYPE_MOUSE`) or nRF Desktop keyboard (:option:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD`).
For more details, see the :file:`src/modules/Kconfig.hids` file.

.. tip::
   If the HID report configuration is identical to the default configuration of either nRF Desktop mouse or keyboard, you do not need to modify the |GATT_HID| configuration.
   Otherwise, see :ref:`hids_readme` documentation for configuration details.

HID subscriber configuration
============================

The HID reports are forwarded to the connected GATT Client after it enables the HID notifications.

On an nRF Desktop peripheral, the :ref:`nrf_desktop_hid_state` handles generating HID reports as per your input and providing the reports to the HID subscriber.
The HID Service application module subscribes for HID reports using :c:struct:`hid_report_subscriber_event` and :c:struct:`hid_report_subscription_event`.
After subscriptions are enabled in |hid_state|, the |hid_state| sends the HID input reports as :c:struct:`hid_report_event`.
The HID Service application module sends the report over Bluetooth LE and submits the :c:struct:`hid_report_sent_event` event once the given HID input report has been sent.

You can use the following Kconfig options to modify HID subscription parameters used in the :c:struct:`hid_report_subscriber_event`:

* :option:`CONFIG_DESKTOP_HIDS_SUBSCRIBER_PRIORITY` (:c:member:`hid_report_subscriber_event.priority`).
* :option:`CONFIG_DESKTOP_HIDS_SUBSCRIBER_REPORT_MAX` (:c:member:`hid_report_subscriber_event.report_max`).

For more details, see the Kconfig help.

.. note::
   For the Bluetooth connections, the information that GATT notification with a HID report was sent is delayed by one Bluetooth LE connection interval.
   Because of this delay, the module always uses pipeline (:c:member:`hid_report_subscriber_event.pipeline_size`) of two sequential HID reports to make sure that data can be sent on every Bluetooth LE connection event.

HID subscription delay
----------------------

By default, the ``hids`` application module starts forwarding the subscriptions right after the Bluetooth connection is secured.
You can define additional delay for forwarding the notifications on connection (:option:`CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY`).
Sending the first HID report to the connected Bluetooth peer is delayed by this period of time.

.. note::
   The nRF Desktop centrals perform the GATT service discovery and reenable the HID notifications on every reconnection.
   A HID report that is received before the subscription is reenabled will be dropped before it reaches the application.
   The :option:`CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY` option is set to 1000 ms for nRF Desktop keyboards (:option:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD`) to make sure that the input is not lost on reconnection with the nRF Desktop dongle.

Implementation details
**********************

The HID Service application module initializes and configures the |GATT_HID|.
The application module registers the HID report map and every HID report that was enabled in the application configuration.
For detailed information about the HID-related configuration in the nRF Desktop, see the :ref:`nrf_desktop_hid_configuration` documentation.

HID keyboard LED output report
==============================

The module can receive a HID output report setting state of the keyboard LEDs, for example, state of the Caps Lock.
The report is received from the Bluetooth connected host.
The module forwards the report using :c:struct:`hid_report_event` that is handled by |hid_state|.

Right now, only development kits in ``keyboard`` build type configuration display information received in the HID output report using hardware LEDs.
The keyboard reference design (nrf52kbd) has only one LED that is used to display the Bluetooth LE peer state.
Detailed information about the usage of LEDs to display information about Bluetooth LE peer state and system state to the user is available in the :ref:`nrf_desktop_led_state` documentation.
Detailed information about displaying state of the HID keyboard LEDs using hardware LEDs is available in :ref:`nrf_desktop_hid_state` documentation.

Bluetooth LE connections and disconnections
===========================================

The module informs the |GATT_HID| about the Bluetooth LE connections and disconnections using :c:func:`bt_hids_connected` and :c:func:`bt_hids_disconnected`, respectively.

Registered handlers
===================

The |GATT_HID| uses registered handlers to send the following information to the HID Service application module:

* Enabling or disabling a HID input report notification.
* Incoming HID output or feature reports.
* Switching between the boot mode and the report mode.

HID notifications
=================

The :c:struct:`hid_notification_event` is used to synchronize the information about enabling or disabling the HID notifications for the HID input report.
The event is submitted when the |GATT_HID| calls a callback related to enabling or disabling the notifications and the event is received only by the HID Service application module.

Transport for configuration channel
===================================

The HID Service application module works as a transport for the :ref:`nrf_desktop_config_channel` and exchanges the |conf_channel| HID reports over Bluetooth LE.
The module communicates with the |conf_channel| listeners using :c:struct:`config_event`.
