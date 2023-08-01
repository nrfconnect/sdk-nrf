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

Complete the following steps to configure the module:

1. Complete the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
   Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` options are enabled.
   The HID Service application module is enabled by the :ref:`CONFIG_DESKTOP_HIDS_ENABLE <config_desktop_app_options>` option, which is implied by :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` together with other GATT Services that are required for a HID device.
#. The :ref:`CONFIG_DESKTOP_HIDS_ENABLE <config_desktop_app_options>` option selects the following Kconfig options:

   * The :kconfig:option:`CONFIG_BT_HIDS` option that automatically enables the :ref:`hids_readme`.
   * The :kconfig:option:`CONFIG_BT_CONN_CTX` option that automatically enables the :ref:`bt_conn_ctx_readme`, which is required by the |GATT_HID|.

#. The nRF Desktop application modifies the defaults of Kconfig option values, defined by the :ref:`hids_readme`, to tailor the default configuration to application needs.
   The configuration is tailored for either nRF Desktop mouse (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_MOUSE <config_desktop_app_options>`) or nRF Desktop keyboard (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD <config_desktop_app_options>`).
   For more details, see the :file:`src/modules/Kconfig.hids` file.

   .. tip::
     If the HID report configuration is identical to the default configuration of either nRF Desktop mouse or keyboard, you do not need to modify the |GATT_HID| configuration.
     Otherwise, see :ref:`hids_readme` documentation for configuration details.

The HID Service application module forwards the information about the enabled HID notifications to other application modules using ``hid_report_subscription_event``.
These notifications are enabled by the connected Bluetooth® Central.
By default, the ``hids`` application module starts forwarding the subscriptions right after the Bluetooth connection is secured.

You can define additional delay for forwarding the notifications on connection (:ref:`CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY <config_desktop_app_options>`).
Sending the first HID report to the connected Bluetooth peer is delayed by this period of time.

.. note::
   The nRF Desktop centrals perform the GATT service discovery and reenable the HID notifications on every reconnection.
   A HID report that is received before the subscription is reenabled will be dropped before it reaches the application.
   The :ref:`CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY <config_desktop_app_options>` option is set to 500 ms for nRF Desktop keyboards (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD <config_desktop_app_options>`) to make sure that the input is not lost on reconnection with the nRF Desktop dongle.

Implementation details
**********************

The HID Service application module initializes and configures the |GATT_HID|.
The application module registers the HID report map and every HID report that was enabled in the application configuration.
For detailed information about the HID-related configuration in the nRF Desktop, see the :ref:`nrf_desktop_hid_configuration` documentation.

Sending HID input reports
=========================

After subscriptions are enabled in |hid_state|, the |hid_state| sends the HID input reports as ``hid_report_event``.
The HID Service application module sends the report over Bluetooth LE and submits the ``hid_report_sent_event`` to inform that the given HID input report was sent.

HID keyboard LED output report
==============================

The module can receive a HID output report setting state of the keyboard LEDs, for example, state of the Caps Lock.
The report is received from the Bluetooth connected host.
The module forwards the report using ``hid_report_event`` that is handled by |hid_state|.

Right now, the only board that displays information received in the HID output report using hardware LEDs is the :ref:`nrf52840dk_nrf52840 <nrf52840dk_nrf52840>` in ``keyboard`` build type configuration.
The keyboard reference design (nrf52kbd_nrf52832) has only one LED that is used to display the Bluetooth LE peer state.
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

The ``hid_notification_event`` is used to synchronize the information about enabling or disabling the HID notifications for the HID input report.
The event is submitted when the |GATT_HID| calls a callback related to enabling or disabling the notifications and the event is received only by the ``hids`` application module.

Transport for configuration channel
===================================

The HID Service application module works as a transport for the :ref:`nrf_desktop_config_channel` and exchanges the |conf_channel| HID reports over Bluetooth LE.
The module communicates with the |conf_channel| listeners using ``config_event``.
