.. _nrf_desktop_hids:

HID Service module
##################

Use the HID Service module to handle the GATT Human Interface Device Service.
The GATT Service is used to exchange HID data with the peer connected over Bluetooth.
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

Complete the following steps to configure the module:

1. Complete the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
   During this configuration, you must enable the :option:`CONFIG_BT_PERIPHERAL` Kconfig option for every nRF Desktop peripheral.
   When this option is enabled, the ``CONFIG_DESKTOP_HID_PERIPHERAL`` is set to ``y``, which enables the following two additional options, among others:

   * :option:`CONFIG_BT_GATT_HIDS` - This is required because the HID Service module is based on the :ref:`hids_readme` implementation of the GATT Service.
   * ``CONFIG_DESKTOP_HIDS_ENABLE`` - This enables the ``hids`` application module.

   This step also enables the |GATT_HID|.
#. Enable the :ref:`bt_conn_ctx_readme` (:option:`CONFIG_BT_CONN_CTX`).
   This is required by the |GATT_HID|.
#. Configure the :ref:`hids_readme`.
   See its documentation for configuration details.

   .. tip::
     If the HID report configuration is identical to the configuration used for one of the existing devices, you can use the same |GATT_HID| configuration.

The HID Service application module forwards the information about the enabled HID notifications to other application modules using ``hid_report_subscription_event``.
These notifications are enabled by the connected Bluetooth Central.
By default, the ``hids`` application module starts forwarding the subscriptions right after the Bluetooth connection is secured.

You can define additional delay for forwarding the notifications on connection (``CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY``).
Sending the first HID report to the connected Bluetooth peer is delayed by this period of time.

.. note::
   The nRF Desktop centrals perform the GATT service discovery and reenable the HID notifications on every reconnection.
   A HID report that is received before the subscription is reenabled will be dropped before it reaches the application.
   The ``CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY`` is used for keyboard reference design (nRF52832 Desktop Keyboard) to make sure that the input will not be lost on reconnection with the nRF Desktop dongle.

Implementation details
**********************

The HID Service application module initializes and configures the |GATT_HID|.
The application module registers the HID report map and every HID report that was enabled in the application configuration.
Detailed information about HID-related configuration in nRF Desktop is available in the :ref:`nrf_desktop_hid_state` documentation.

Sending HID input reports
=========================

After subscriptions are enabled in |HID_state|, the |HID_state| sends the HID input reports as ``hid_report_event``.
The HID Service application module sends the report over Bluetooth LE and submits the ``hid_report_sent_event`` to inform that the given HID input report was sent.

Ignored LED HID output reports
==============================

The HID output reports for keyboard LEDs are ignored by the module, because the nRF Desktop keyboard does not support them.
The keyboard has only one LED that is used to display Bluetooth LE peer state.
Detailed information about the usage of LEDs to display information to the user is available in the :ref:`nrf_desktop_led_state` documentation.

Bluetooth LE connections and disconnections
===========================================

The module informs the |GATT_HID| about the Bluetooth LE connections and disconnections using :c:func:`bt_gatt_hids_notify_connected` and :c:func:`bt_gatt_hids_notify_disconnected`, respectively.

Registered handlers
===================

The |GATT_HID| uses registered handlers to send the following information to the HID Service application module:

* Enabling or disabling a HID input report notification.
* Incoming HID output or feature reports.
* Switching between the boot mode and the report mode.

HID notifications
=================

The ``hid_notification_event`` is used to synchronize the information about enabling or disabling the HID notifications for the HID input report.
The event is submitted when the |GATT_HID| calls a callback related to enabling or disabling the notifications and the event is received only by the ``hids`` application module.

Transport for configuration channel
===================================

The HID Service application module works as a transport for the :ref:`nrf_desktop_config_channel` and exchanges the |conf_channel| HID reports over Bluetooth LE.
The module communicates with the |conf_channel| listeners using ``config_event``.

.. |GATT_HID| replace:: GATT HID Service
.. |HID_state| replace:: HID state module
.. |conf_channel| replace:: Configuration channel
