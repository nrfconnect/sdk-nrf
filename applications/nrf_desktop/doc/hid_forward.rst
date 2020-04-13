.. _nrf_desktop_hid_forward:

HID forward module
##################

Use the HID forward module to:

* receive the HID input reports from the Bluetooth connected peripherals.
* forward the :ref:`nrf_desktop_config_channel` data between the Bluetooth connected peripherals and the host.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_forward_start
    :end-before: table_hid_forward_end

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

Complete the following steps to configure the module:

1. Complete the basic Bluetooth configuration, as described in the nRF Desktop Bluetooth guide.
#. Enable and configure the :ref:`hids_c_readme` (:option:`CONFIG_BT_GATT_HIDS_C`).
   Make sure to define the maximum number of supported HID reports (:option:`CONFIG_BT_GATT_HIDS_C_REPORTS_MAX`).
#. The ``hid_forward`` module is enabled with ``CONFIG_DESKTOP_HID_FORWARD_ENABLE`` option.
   The option is selected by default for every nRF Desktop central - by default, it is set to ``y`` if :option:`CONFIG_BT_CENTRAL` option is enabled.

Implementation details
**********************

Forwarding HID input reports
   After :ref:`nrf_desktop_ble_discovery` successfully discovers a connected peripheral, the ``hid_forward`` module automatically subscribes for every HID input report provided by the peripheral.

   The :cpp:func:`hidc_read` callback is called when HID input report is received from the connected peripheral.
   The received HID input report data is converted to ``hid_report_event``.

   The ``hid_report_event`` is submitted and then the :ref:`nrf_desktop_usb_state` forwards it to the host.
   When a HID report is sent to the host, the ``hid_forward`` module receives a ``hid_report_sent_event``.

Enqueuing incoming HID input reports
   At a time the device forwards only one HID input report to the host.
   Another HID input report may be received from a Bluetooth connected peripheral before the previous one was sent to the host.
   In that case ``hid_report_event`` is enqueued and submitted later.
   Up to :c:macro:`MAX_ENQUEUED_ITEMS` reports can be enqueued at a time.
   In case there is no space to enqueue a new event, the module drops the oldest enqueued event.

   Upon receiving the ``hid_report_sent_event``, ``hid_forward`` submits the first enqueued ``hid_report_event``.
   If there is no ``hid_report_event`` in the queue, the module waits for receiving data from peripherals.

Bluetooth peripheral disconnection
   On a peripheral disconnection, nRF Desktop central informs the host that all the pressed keys reported by the peripheral are released.
   This is done to make sure that user will not observe a problem with a key stuck on peripheral disconnection.

Configuration channel data forwarding
   The ``hid_forward`` module forwards the :ref:`nrf_desktop_config_channel` data between the host and the Bluetooth connected peripherals.
   The data forwarding is based on peripheral device Product ID (PID).
   The data is exchanged with the Bluetooth connected peripheral using HID feature reports.

Bluetooth connection latency control
   If the Bluetooth central uses nrfxlib Link Layer (:option:`CONFIG_BT_LL_NRFXLIB`), the connection latency updates must be triggered by the central.
   In that case, the ``hid_forward`` module updates the connection latency to keep it low if the :ref:`nrf_desktop_config_channel` is in use.
   If the nRF Desktop peripheral is connected with an external Bluetooth central, this task is handled by the :ref:`nrf_desktop_ble_latency` module on the peripheral.
