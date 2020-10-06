.. _nrf_desktop_hid_forward:

HID forward module
##################

Use the HID forward module to:

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
#. Enable and configure the :ref:`hids_c_readme` (:option:`CONFIG_BT_GATT_HIDS_C`).

   .. note::
       Make sure to define the maximum number of supported HID reports (:option:`CONFIG_BT_GATT_HIDS_C_REPORTS_MAX`).

#. Make sure that the ``CONFIG_DESKTOP_HID_STATE_ENABLE`` option is disabled.
   The nRF Desktop central does not generate its own HID input reports.
   It only forwards HID input reports that it receives from the peripherals connected over Bluetooth.
#. The HID forward module is enabled with the ``CONFIG_DESKTOP_HID_FORWARD_ENABLE`` option.
   The option is selected by default for every nRF Desktop central.
   By default, it is set to ``y`` if the :option:`CONFIG_BT_CENTRAL` option is enabled.

You can set the queued HID input reports limit using the ``CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS`` Kconfig option.

Implementation details
**********************

See the following sections for considerations related to the functioning of the HID forward module.

Forwarding HID input reports
============================

After :ref:`nrf_desktop_ble_discovery` successfully discovers a connected peripheral, the HID forward module automatically subscribes for every HID input report provided by the peripheral.

The :c:func:`hidc_read` callback is called when HID input report is received from the connected peripheral.
The received HID input report data is converted to ``hid_report_event``.

``hid_report_event`` is submitted and then the :ref:`nrf_desktop_usb_state` forwards it to the host.
When a HID report is sent to the host, the ``hid_forward`` module receives a ``hid_report_sent_event``.

Enqueuing incoming HID input reports
====================================

The device forwards only one HID input report to the host at a time.
Another HID input report may be received from a peripheral connected over Bluetooth before the previous one was sent to the host.
In that case, ``hid_report_event`` is enqueued and submitted later.
Up to ``CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS`` reports can be enqueued at a time.
In case there is no space to enqueue a new event, the module drops the oldest enqueued event.

Upon receiving the ``hid_report_sent_event``, ``hid_forward`` submits the first enqueued ``hid_report_event``.
If there is no ``hid_report_event`` in the queue, the module waits for receiving data from peripherals.

Bluetooth Peripheral disconnection
==================================

On a peripheral disconnection, nRF Desktop central informs the host that all the pressed keys reported by the peripheral are released.
This is done to make sure that user will not observe a problem with a key stuck on peripheral disconnection.

Configuration channel data forwarding
=====================================

The ``hid_forward`` module forwards the :ref:`nrf_desktop_config_channel` data between the host and the peripherals connected over Bluetooth.
The data is exchanged with the peripheral connected over Bluetooth using HID feature report or HID output report.

In contrast to :ref:`nrf_desktop_config_channel_script`, the ``hid_forward`` module does not use configuration channel request to get hardware ID (HW ID) of the peripheral.
The peripheral indentification on nRF Desktop central is based on HW ID that is received from :ref:`nrf_desktop_ble_discovery` when peripheral discovery is completed.
The peripheral uses :ref:`nrf_desktop_dev_descr` to provide the HW ID to the nRF Desktop central.

The ``hid_forward`` only forwards the configuration channel requests that come from the USB connected host, it does not generate its own requests.

For more details, see :ref:`nrf_desktop_config_channel` documentation.
