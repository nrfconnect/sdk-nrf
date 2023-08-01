.. _nrf_desktop_usb_state:

USB state module
################

.. contents::
   :local:
   :depth: 2

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

The module is enabled by selecting :ref:`CONFIG_DESKTOP_USB_ENABLE <config_desktop_app_options>` option.
The option selects :kconfig:option:`CONFIG_USB_DEVICE_STACK` and :kconfig:option:`CONFIG_USB_DEVICE_HID` Kconfig options.

The :ref:`CONFIG_DESKTOP_USB_ENABLE <config_desktop_app_options>` option is implied by the :ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>` option.
The module is enabled by default for the nRF Desktop dongles because the dongles forward the HID data to the host connected over USB.
See the :ref:`nrf_desktop_hid_configuration` documentation for details.

Additionally, you can also configure the options described in the following sections.

.. _nrf_desktop_usb_state_identifiers:

USB device identifiers
======================

When the USB support is enabled for the device, the following options are by default set to common :ref:`nrf_desktop_hid_device_identifiers` defined for the nRF Desktop application:

* :kconfig:option:`CONFIG_USB_DEVICE_MANUFACTURER` - Manufacturer's name.
* :kconfig:option:`CONFIG_USB_DEVICE_PRODUCT` - Product name.
* :kconfig:option:`CONFIG_USB_DEVICE_VID` - Vendor ID (VID) number.
* :kconfig:option:`CONFIG_USB_DEVICE_PID` - Product ID (PID) number.

Low latency device configuration
================================

The module sets default value of :kconfig:option:`CONFIG_USB_HID_POLL_INTERVAL_MS` to ``1``.
For low latency of HID reports, the device requests a polling rate of 1 ms.

Boot protocol configuration
===========================

The module sets default values of :kconfig:option:`CONFIG_USB_HID_BOOT_PROTOCOL` and :kconfig:option:`CONFIG_USB_HID_PROTOCOL_CODE` Kconfig options to support boot reports defined in the nRF Desktop configuration.
The :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD <config_desktop_app_options>` are used to control support of HID boot reports in the nRF Desktop application.

USB device instance configuration
=================================

The nRF Desktop device can provide multiple instances of a HID-class USB device.
The number of instances is controlled by :kconfig:option:`CONFIG_USB_HID_DEVICE_COUNT`.
By default, the option is set to:

* ``1`` for HID peripherals (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`).
* :ref:`CONFIG_DESKTOP_HID_DONGLE_BOND_COUNT <config_desktop_app_options>` for HID dongles (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`).

See the subsections below for details.

nRF Desktop Peripheral
----------------------

The nRF Desktop Peripheral devices by default use only a single HID-class USB instance.
In that case, this instance is used for all the HID reports.

Enable :ref:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION <config_desktop_app_options>` to use more than one HID-class USB instance on nRF Desktop Peripheral.
Make sure to set a greater value in the :kconfig:option:`CONFIG_USB_HID_DEVICE_COUNT` option and create an additional :file:`usb_state_def.h` header in the configuration.
The header assigns HID reports to the HID-class USB instances.
A given HID report can be handled only by a single HID-class USB instance.
For example, the file contents can look as follows:

   .. code-block:: c

      #include "hid_report_desc.h"

      /* This configuration file is included only once from usb_state module and holds
       * information about HID report subscriptions of USB HID instances.
       */

      /* This structure enforces the header file is included only once in the build.
       * Violating this requirement triggers a multiple definition error at link time.
       */
      const struct {} usb_state_def_include_once;

      static const uint32_t usb_hid_report_bm[] = {
             BIT(REPORT_ID_MOUSE),
             BIT(REPORT_ID_KEYBOARD_KEYS),
      };

The ``usb_hid_report_bm`` defines HID reports handled by a HID-class USB instance, in a bitmask format.
In this example, the HID mouse input report is handled by the first HID-class USB instance and the HID keyboard input report is handled by the second HID-class USB instance.

nRF Desktop Central
-------------------

The nRF Desktop Central device can use either a single HID-class USB instance or a number of instances equal to that specified in the :ref:`CONFIG_DESKTOP_HID_DONGLE_BOND_COUNT <config_desktop_app_options>` option.
If only one instance is used, reports from all Peripherals connected to the Central are forwarded to the same instance.
In other cases, reports from each of the bonded peripherals are forwarded to a dedicated HID-class USB instance.
The same instance is used after reconnection.

USB wakeup configuration
========================

The nRF Desktop device can work as a source of wakeup events for the host device if connected through the USB.
To use the feature, enable the :ref:`CONFIG_DESKTOP_USB_REMOTE_WAKEUP <config_desktop_app_options>` option.
This option selects the :kconfig:option:`CONFIG_USB_DEVICE_REMOTE_WAKEUP` to enable required dependencies in the USB stack.

When host enters the suspended state, the USB will be suspended as well.
With this feature enabled, this state change is used to suspend the nRF Desktop device (see :ref:`nrf_desktop_power_manager`).
When the nRF Desktop device wakes up from standby, the |usb_state| will issue a wakeup request on the USB.

.. note::
    The USB wakeup request is transmitted to the host only if the host enables this request before suspending the USB.

Implementation details
**********************

The |usb_state| registers the :kconfig:option:`CONFIG_USB_HID_DEVICE_COUNT` instances of HID-class USB device and initializes the USB subsystem.

The necessary callbacks are connected to the module to ensure that the state of the USB connection is tracked.
From the application's viewpoint, USB can be in the following states:

* :c:enum:`USB_STATE_DISCONNECTED` - USB cable is not connected.
* :c:enum:`USB_STATE_POWERED` - The device is powered from USB but is not configured for the communication.
* :c:enum:`USB_STATE_ACTIVE` - The device is ready to exchange data with the host.
* :c:enum:`USB_STATE_SUSPENDED` - The host has requested the device to enter the suspended state.

These states are broadcast by the |usb_state| with a :c:struct:`usb_state_event`.
When the device is connected to the host and configured for the communication, the module will broadcast the :c:enum:`USB_STATE_ACTIVE` state.
The module will also subscribe to all HID reports available in the application for the selected protocol.

When the device is disconnected from the host, the module will unsubscribe from receiving the HID reports.

When the HID report data is transmitted through :c:struct:`hid_report_event`, the module will pass it to the associated endpoint.
Upon data delivery, :c:struct:`hid_report_sent_event` is submitted by the module.

.. note::
    Only one report can be transmitted by the module to a single instance of HID-class USB device at any given time.
    Different instances can transmit reports in parallel.

The |usb_state| is a transport for :ref:`nrf_desktop_config_channel` when the channel is enabled.

The module also handles a HID keyboard LED output report received through USB from the connected host.
The module sends the report using :c:struct:`hid_report_event`, that is handled either by :ref:`nrf_desktop_hid_state` (for peripheral) or by the :ref:`nrf_desktop_hid_forward` (for dongle).
