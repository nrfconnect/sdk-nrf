.. _nrf_desktop_hid_forward:

HID forward module
##################

.. contents::
   :local:
   :depth: 2

Use the |hid_forward| for the following purposes:

* Receive the HID input reports from the peripherals connected over Bluetooth®.
* Forward the HID input reports in report or boot protocol.
* Forward the :ref:`nrf_desktop_config_channel` data between the peripherals connected over Bluetooth and the host.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_forward_start
    :end-before: table_hid_forward_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Complete the following steps to configure the module:

1. Complete the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
   Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` are enabled.
   The HID forward application module is enabled by the :ref:`CONFIG_DESKTOP_HID_FORWARD_ENABLE <config_desktop_app_options>` option which is implied by the :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` option together with other application modules.
   These modules are required for HID dongle that forwards the data from HID peripherals connected over Bluetooth.
#. The :ref:`CONFIG_DESKTOP_HID_FORWARD_ENABLE <config_desktop_app_options>` option selects :kconfig:option:`CONFIG_BT_HOGP` to automatically enable the :ref:`hogp_readme`.
   An nRF Desktop dongle does not generate its own HID input reports.
   The dongle uses |hid_forward| to forward the HID reports.
   The reports are received by the HID service client from the peripherals connected over Bluetooth.

   .. note::
       The maximum number of supported HID reports (:kconfig:option:`CONFIG_BT_HOGP_REPORTS_MAX`) is set by default for the nRF Desktop dongle, which supports two peripherals with an average of six HID reports each.
       Make sure to align this configuration value for other use cases, for example, if the dongle supports more peripherals.

#. nRF Desktop dongle can forward either mouse or keyboard boot reports.
   The forwarded boot report type is specified using the following Kconfig options:

   * :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD <config_desktop_app_options>` - This option enables forwarding keyboard boot reports.
   * :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>` - This option enables forwarding mouse boot reports.

   Those options affect :ref:`nrf_desktop_usb_state` that subscribes for HID boot reports.
   The Dongle forwards HID reports from both mouse and keyboard, and so either option works if you want to have the Dongle work as boot mouse or boot keyboard.
   For more information about the configuration of the HID boot protocol, see the boot protocol configuration section in the :ref:`nrf_desktop_usb_state` documentation.

You can set the queued HID input reports limit using the :ref:`CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS <config_desktop_app_options>` Kconfig option.

Implementation details
**********************

This section describes the functioning of the |hid_forward|.

Interaction with the USB
========================

The :ref:`nrf_desktop_usb_state` can be configured to have one or more instances of the HID-class USB device.
If there is more than one instance of the HID-class USB device, this number must match the maximum number of bonded Bluetooth peripheral devices.
Each instance of HID-class USB device subscribes to HID reports forwarded by the |hid_forward|.

The |hid_forward| has an array of subscribers, one for each HID-class USB device.
The possible cases that impact how the host to which the nRF desktop dongle is connected interprets the reports are as follows:

* Number of HID-class USB devices is equal to number of bonded peripheral devices.
  In this case:

  * Reports from every bonded peripheral will be forwarded to a dedicated HID-class USB device.
  * The host can distinguish the source of the report.

* Only one HID-class USB device is used.
  In this case:

  * Reports from all peripherals will be forwarded to a single HID-class USB device.
  * The host cannot distinguish the source of the report.

When reports are forwarded to the :ref:`nrf_desktop_usb_state`, each HID-class USB device is treated independently.
This means that the |hid_forward| can concurrently send reports to every available HID-class USB device, if a report from the linked peripheral is available.

When received by the module, the HID boot protocol reports come with the report ID assigned to :c:enum:`REPORT_ID_RESERVED`.
This ID is later changed by the |hid_forward| to either :c:enum:`REPORT_ID_BOOT_KEYBOARD` or :c:enum:`REPORT_ID_BOOT_MOUSE`.
The choice depends on fact, if the Bluetooth device is boot mouse or keyboard.
While receiving a subscription event, the |hid_forward| updates the HID protocol of peripherals connected through Bluetooth that are associated with the given subscriber.

Forwarding HID input reports
============================

After :ref:`nrf_desktop_ble_discovery` successfully discovers a connected peripheral, the |hid_forward| automatically subscribes for every HID input report provided by the peripheral.
The subscriber can use either the HID boot protocol or the HID report protocol, but both protocols cannot be used at the same time.
In the current implementation, the :ref:`nrf_desktop_usb_state` supports either the HID boot keyboard or the HID boot mouse reports, because the HID boot protocol code is set using a single Kconfig option that is common for all of the instances of the USB HID.

The :c:func:`hogp_read` callback is called when HID input report is received from the connected peripheral.
The received HID input report data is converted to ``hid_report_event``.

``hid_report_event`` is submitted and then the HID-class USB device configured by :ref:`nrf_desktop_usb_state` forwards it to the host.
When a HID report is sent to the host by the HID-class USB device, the |hid_forward| receives a ``hid_report_sent_event`` with the identifier of this device.

Enqueuing incoming HID input reports
------------------------------------

The |hid_forward| forwards only one HID input report to the HID-class USB device at a time.
Another HID input report may be received from a peripheral connected over Bluetooth before the previous one was sent.
In that case, ``hid_report_event`` is enqueued and submitted later.
Up to the number of reports specified in :ref:`CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS <config_desktop_app_options>` reports can be enqueued at a time for each report type and for each connected peripheral.
If there is not enough space to enqueue a new event, the module drops the oldest enqueued event that was received from this peripheral (of the same type).

Upon receiving the ``hid_report_sent_event``, the |hid_forward| submits the ``hid_report_event`` enqueued for the peripheral that is associated with the HID-class USB device.
The enqueued report to be sent is chosen by the |hid_forward| in the round-robin fashion.
The report of the next type will be sent if available.
If not available, the next report type will be checked until a report is found or there is no report in any of the queues.
If there is no ``hid_report_event`` in the queue, the module waits for receiving data from peripherals.

Forwarding HID output reports
=============================

When the |hid_forward| receives a ``hid_report_event`` that contains an output report from a :ref:`nrf_desktop_usb_state`, it tries to forward the output report.
The HID output report is forwarded to all of the Bluetooth connected peripherals that forward the HID data to the HID subscriber that is source of the HID output report.
The HID output report is never forwarded to peripheral that does not support it.

If the GATT write without response operation is in progress for the given HID output report ID and connected peripheral, the report is placed in a queue and sent later.
The |hid_forward| sends information only about the last received HID output report with given ID.
If changes of state related to the HID output report with the given ID are frequent, some intermediate states can be omitted by the |hid_forward|.

Bluetooth Peripheral disconnection
==================================

On a peripheral disconnection, nRF Desktop central informs the host that all the pressed keys reported by the peripheral are released.
This is done to make sure that the user will not observe a problem with a key stuck on peripheral disconnection.

Configuration channel data forwarding
=====================================

The |hid_forward| forwards the :ref:`nrf_desktop_config_channel` data between the host and the peripherals connected over Bluetooth.
The data is exchanged with the peripheral connected over Bluetooth using HID feature report or HID output report.

In contrast to :ref:`nrf_desktop_config_channel_script`, the |hid_forward| does not use configuration channel request to get hardware ID (HW ID) of the peripheral.
The peripheral identification on nRF Desktop central is based on the HW ID that is received from :ref:`nrf_desktop_ble_discovery` when peripheral discovery is completed.
The peripheral uses :ref:`nrf_desktop_dev_descr` to provide the HW ID to the nRF Desktop central.

The |hid_forward| only forwards the configuration channel requests that come from the USB connected host, it does not generate its own requests.

For more details, see :ref:`nrf_desktop_config_channel` documentation.
