.. _nrf_desktop_hid_forward:

HID forward module
##################

.. contents::
   :local:
   :depth: 2

Use the |hid_forward| to:

* Receive the HID input reports from the peripherals connected over Bluetooth.
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
#. Enable and configure the :ref:`hogp_readme` (:option:`CONFIG_BT_HOGP`).

   .. note::
       Make sure to define the maximum number of supported HID reports (:option:`CONFIG_BT_HOGP_REPORTS_MAX`).

#. Make sure that the :option:`CONFIG_DESKTOP_HID_STATE_ENABLE` option is disabled.
   The nRF Desktop central does not generate its own HID input reports.
   It only forwards HID input reports that it receives from the peripherals connected over Bluetooth.
#. The |hid_forward| is enabled with the :option:`CONFIG_DESKTOP_HID_FORWARD_ENABLE` option.
   This option is available only if you enable both the :option:`CONFIG_BT_CENTRAL` and :option:`CONFIG_BT_HOGP` Kconfig options.
   The module is used on Bluetooth Central to forward the HID reports that are received by the HID service client.

You can set the queued HID input reports limit using the :option:`CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS` Kconfig option.

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

Forwarding HID input reports
============================

After :ref:`nrf_desktop_ble_discovery` successfully discovers a connected peripheral, the |hid_forward| automatically subscribes for every HID input report provided by the peripheral.

The :c:func:`hidc_read` callback is called when HID input report is received from the connected peripheral.
The received HID input report data is converted to ``hid_report_event``.

``hid_report_event`` is submitted and then the HID-class USB device configured by :ref:`nrf_desktop_usb_state` forwards it to the host.
When a HID report is sent to the host by the HID-class USB device, the |hid_forward| receives a ``hid_report_sent_event`` with the identifier of this device.

Enqueuing incoming HID input reports
====================================

The |hid_forward| forwards only one HID input report to the HID-class USB device at a time.
Another HID input report may be received from a peripheral connected over Bluetooth before the previous one was sent.
In that case, ``hid_report_event`` is enqueued and submitted later.
Up to :option:`CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS` reports can be enqueued at a time for each report type and for each connected peripheral.
If there is not enough space to enqueue a new event, the module drops the oldest enqueued event that was received from this peripheral (of the same type).

Upon receiving the ``hid_report_sent_event``, the |hid_forward| submits the ``hid_report_event`` enqueued for the peripheral that is associated with the HID-class USB device.
The enqueued report to be sent is chosen by the |hid_forward| in the round-robin fashion.
The report of the next type will be sent if available.
If not available, the next report type will be checked until a report is found or there is no report in any of the queues.
If there is no ``hid_report_event`` in the queue, the module waits for receiving data from peripherals.

Bluetooth Peripheral disconnection
==================================

On a peripheral disconnection, nRF Desktop central informs the host that all the pressed keys reported by the peripheral are released.
This is done to make sure that user will not observe a problem with a key stuck on peripheral disconnection.

Configuration channel data forwarding
=====================================

The |hid_forward| forwards the :ref:`nrf_desktop_config_channel` data between the host and the peripherals connected over Bluetooth.
The data is exchanged with the peripheral connected over Bluetooth using HID feature report or HID output report.

In contrast to :ref:`nrf_desktop_config_channel_script`, the |hid_forward| does not use configuration channel request to get hardware ID (HW ID) of the peripheral.
The peripheral indentification on nRF Desktop central is based on HW ID that is received from :ref:`nrf_desktop_ble_discovery` when peripheral discovery is completed.
The peripheral uses :ref:`nrf_desktop_dev_descr` to provide the HW ID to the nRF Desktop central.

The |hid_forward| only forwards the configuration channel requests that come from the USB connected host, it does not generate its own requests.

For more details, see :ref:`nrf_desktop_config_channel` documentation.

.. |hid_forward| replace:: HID forward module
