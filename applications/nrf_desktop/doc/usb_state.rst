.. _nrf_desktop_usb_state:

USB state module
################

The |usb_state| is responsible for tracking the USB connection.
It is also responsible for transmitting data through USB on the application's device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_usb_state_start
    :end-before: table_usb_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module is enabled by selecting ``CONFIG_DESKTOP_USB_ENABLE``.
It depends on :option:`CONFIG_USB_DEVICE_HID`.

When enabling the USB support for the device, set the following generic device options:

* :option:`CONFIG_USB_DEVICE_MANUFACTURER` - Manufacturer's name.
* :option:`CONFIG_USB_DEVICE_PRODUCT` - Product name.
* :option:`CONFIG_USB_DEVICE_VID` - Vendor ID (VID) number.
* :option:`CONFIG_USB_DEVICE_PID` - Product ID (PID) number.

For low latency devices, make sure that the device requests a polling rate of 1 ms by setting :option:`CONFIG_USB_HID_POLL_INTERVAL_MS` to ``1``.

If the device is meant to support the boot protocol, select :option:`CONFIG_USB_HID_BOOT_PROTOCOL` and make sure :option:`CONFIG_USB_HID_PROTOCOL_CODE` chooses either the mouse or the keyboard code.

Implementation details
**********************

The |usb_state| registers the HID-class USB device and initializes the USB subsystem.

The necessary callbacks are connected to the module to ensure that the state of the USB connection is tracked.
From the application's viewpoint, USB can be in the following states:

* USB_STATE_DISCONNECTED - USB cable is not connected.
* USB_STATE_POWERED - The device is powered from USB but is not configured for the communication.
* USB_STATE_ACTIVE - The device is ready to exchange data with the host.
* USB_STATE_SUSPENDED - The host has requested the device to enter the suspended state.

These states are broadcast by the |usb_state| with a ``usb_state_event``.
When the device is connected to the host and configured for the communication, the module will broadcast the ``USB_STATE_ACTIVE`` state.
The module will also subscribe to all HID reports available in the application for the selected protocol.

When the device is disconnected from the host, the module will unsubscribe from receiving the HID reports.

When the HID report data is transmitted through ``hid_report_event``, the module will pass it to the associated endpoint.
Upon data delivery, ``hid_report_sent_event`` is submitted by the module.

.. note::
    Only one report can be transmitted by the module at any given time.

.. warning::
    Writing to an endpoint is a blocking operation.

The |usb_state| is a transport for :ref:`nrf_desktop_config_channel` when the channel is enabled.

.. |usb_state| replace:: USB state module
