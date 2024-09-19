.. _nrf_desktop_usb_state:

USB state module
################

.. contents::
   :local:
   :depth: 2

The |usb_state| is responsible for tracking the USB connection.
It is also responsible for transmitting HID data through USB on the application's device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_usb_state_start
    :end-before: table_usb_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module is enabled by selecting the :ref:`CONFIG_DESKTOP_USB_ENABLE <config_desktop_app_options>` option.

The :ref:`CONFIG_DESKTOP_USB_ENABLE <config_desktop_app_options>` option is implied by the :ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>` option.
The module is enabled by default for the nRF Desktop dongles because the dongles forward the HID data to the host connected over USB.
See the :ref:`nrf_desktop_hid_configuration` documentation for details.

The module can be used with either the USB legacy stack or the USB next stack.
The USB stack is selected by enabling one of the following Kconfig choice options:

* :ref:`CONFIG_DESKTOP_USB_STACK_LEGACY <config_desktop_app_options>`
  The option enables USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
  The USB legacy stack is used by default.
* :ref:`CONFIG_DESKTOP_USB_STACK_NEXT <config_desktop_app_options>`
  The option enables USB next stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`).
  This is the only USB stack that supports USB High-Speed and the nRF54H20 platform.

.. note::
  The USB next stack integration is :ref:`experimental <software_maturity>`.
  The HID boot protocol integration is not yet fully tested and may not work properly.
  The secondary image slot background erase in :ref:`nrf_desktop_dfu` may cause missing USB HID subscriptions after a USB cable is attached.

Some of the properties are configured in the same way for both stacks, but part of the configuration is specific to the selected USB stack.
See the following sections for details.

.. _nrf_desktop_usb_state_identifiers:

USB device identifiers
======================

USB device identifiers are defined in the same way for both USB stacks.
The module relies on common :ref:`nrf_desktop_hid_device_identifiers` defined for the nRF Desktop application.

For the USB legacy stack, this results in updating defaults for the following Kconfig options:

* :kconfig:option:`CONFIG_USB_DEVICE_MANUFACTURER` - Manufacturer's name.
* :kconfig:option:`CONFIG_USB_DEVICE_PRODUCT` - Product name.
* :kconfig:option:`CONFIG_USB_DEVICE_VID` - Vendor ID (VID) number.
* :kconfig:option:`CONFIG_USB_DEVICE_PID` - Product ID (PID) number.

For the USB next stack, the module uses dedicated APIs to set the USB device identifiers during initialization.

.. _nrf_desktop_usb_state_sof_synchronization:

USB Start of Frame (SOF) synchronization
========================================

The module receives a HID input report as :c:struct:`hid_report_event` and submits the report to the USB stack.
The module informs that the HID report was sent using :c:struct:`hid_report_sent_event`.

* If the :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option is disabled, the :c:struct:`hid_report_sent_event` is instantly submitted when a HID report is sent over a USB during USB poll (on USB endpoint read).
  This approach results in shorter HID data latency as a HID report pipeline is not used.
  However, the USB peripheral might not provide a HID report during a USB poll if two subsequent USB polls for HID data happen in quick succession.
  USB polls for HID data are not guaranteed to be evenly spaced in time.
* If the :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option is enabled, submitting :c:struct:`hid_report_sent_event` is delayed until subsequent USB SOF is reported by the USB stack.
  USB SOFs, in contrast to USB polls, are always evenly spaced in time.
  Enabling the Kconfig option reduces the negative impact of jitter related to USB polls and ensures that a HID peripheral can provide a HID input report during every USB poll.
  However, the feature also increases HID data latency as a HID report pipeline with two sequential reports is used.
  Without the pipeline, a USB poll happening quickly after a USB SOF might result in no HID report provided by the peripheral because the HID report source would be unable to provide a subsequent HID input report in time.

  In the case of :ref:`nrf_desktop_hid_mouse_report_handling`, enabling the USB SOF synchronization also synchronizes motion sensor sampling with the USB SOF instead of USB polls (motion sensor sampling is synchronized to :c:struct:`hid_report_sent_event`).
  This synchronization ensures that the sensor is sampled more evenly.

.. _nrf_desktop_usb_state_hid_class_instance:

USB HID class instance configuration
====================================

The nRF Desktop device can provide multiple instances of HID-class USB.
By default, the number of the HID-class USB instances should be set to:

* ``1`` for HID peripherals (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`).
  You can use more HID-class USB instances if the selective HID report subscription feature is enabled (:ref:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION <config_desktop_app_options>`).
* :ref:`CONFIG_DESKTOP_HID_DONGLE_BOND_COUNT <config_desktop_app_options>` for HID dongles (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`).

See the subsections below for details.

nRF Desktop peripheral
----------------------

The nRF Desktop peripheral devices by default use only a single HID-class USB instance.
In that case, this instance is used for all the HID reports.

Enable :ref:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION <config_desktop_app_options>` to use more than one USB HID-class instance on the nRF Desktop peripheral.
Make sure to configure additional HID-class USB instances and create an additional :file:`usb_state_def.h` header in the configuration.
The header assigns HID input reports to the HID-class USB instances.
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

nRF Desktop dongle
------------------

The nRF Desktop dongle device can use either a single HID-class USB instance or a number of instances equal to that specified in the :ref:`CONFIG_DESKTOP_HID_DONGLE_BOND_COUNT <config_desktop_app_options>` option.
If only one instance is used, reports from all peripherals connected to the dongle are forwarded to the same instance.
In other cases, reports from each of the bonded peripherals are forwarded to a dedicated HID-class USB instance.
The same instance is used after reconnection.

USB HID configuration in USB legacy stack
-----------------------------------------

For the USB legacy stack, the :ref:`CONFIG_DESKTOP_USB_STACK_LEGACY <config_desktop_app_options>` selects the required dependencies: :kconfig:option:`CONFIG_USB_DEVICE_STACK` and :kconfig:option:`CONFIG_USB_DEVICE_HID` Kconfig options.
The number of HID-class USB instances in the USB legacy stack is specified by :kconfig:option:`CONFIG_USB_HID_DEVICE_COUNT`.
The default value of this option is updated to match requirements for either an nRF Desktop peripheral or a dongle.

Low latency device configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The module sets the default value of :kconfig:option:`CONFIG_USB_HID_POLL_INTERVAL_MS` to ``1``.
For low latency of HID reports, the device requests a polling rate of 1 ms.

Boot protocol configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD <config_desktop_app_options>` Kconfig options are used to control support of HID boot reports in the nRF Desktop application.
The module aligns the USB HID boot protocol configuration with the application configuration.
The module sets the default value of the :kconfig:option:`CONFIG_USB_HID_BOOT_PROTOCOL` Kconfig option and calls the :c:func:`usb_hid_set_proto_code` function during initialization to set the USB HID boot interface protocol code.
Common HID boot protocol code is used for all of the HID-class USB instances.

USB HID configuration in USB next stack
---------------------------------------

For the USB next stack, the :ref:`CONFIG_DESKTOP_USB_STACK_NEXT <config_desktop_app_options>` selects the :kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT` Kconfig option.
Every USB HID-class instance is configured through a separate DTS node compatible with ``zephyr,hid-device``.
The DTS node configures, among others, the used HID boot protocol, the size of the longest HID input report, and the HID polling rate.
You can configure your preferred USB HID polling rate using the ``in-polling-period-us`` property of the DTS node.
The lowest polling rate that is supported by the USB High-Speed is 125 µs, which corresponds to 8 kHz report rate.
The lowest polling rate supported by devices that do not support USB High-Speed is 1000 µs, which corresponds to 1 kHz report rate.
Make sure to update the DTS configuration to match requirements of your application.
nRF Desktop application defines the USB HID-class instances used by the USB next stack for all of the supported boards and file suffixes.
See the existing configurations for reference.

Defining USB HID-class instances in the DTS, by default, enables the :kconfig:option:`CONFIG_USBD_HID_SUPPORT` Kconfig option.

USB wakeup configuration
========================

The nRF Desktop device can work as a source of wakeup events for the host device if connected through the USB.
To use the feature, enable the :ref:`CONFIG_DESKTOP_USB_REMOTE_WAKEUP <config_desktop_app_options>` option.
This option selects the :kconfig:option:`CONFIG_USB_DEVICE_REMOTE_WAKEUP` Kconfig option to enable required dependencies if USB legacy stack is used.
The USB next stack supports remote wakeup by default, and you do not need to select any additional Kconfig dependencies.

When host enters the suspended state, the USB will be suspended as well.
This state change is used to suspend the nRF Desktop device (see :ref:`nrf_desktop_power_manager`).
When the nRF Desktop device wakes up from standby, the |usb_state| will issue a wakeup request on the USB.

.. note::
    The USB wakeup request is transmitted to the host only if the host enables this request before suspending the USB.

Implementation details
**********************

The |usb_state| registers all of the configured instances of HID-class USB and initializes the USB subsystem.

USB state tracking
==================

The necessary callbacks are connected to the module to ensure that the state of the USB connection is tracked.
From the application's viewpoint, USB can be in the following states:

* :c:enum:`USB_STATE_DISCONNECTED` - USB cable is not connected.
* :c:enum:`USB_STATE_POWERED` - The device is powered from USB but is not configured for the communication.
* :c:enum:`USB_STATE_ACTIVE` - The device is ready to exchange data with the host.
* :c:enum:`USB_STATE_SUSPENDED` - The host has requested the device to enter the suspended state.

These states are broadcasted by the |usb_state| with a :c:struct:`usb_state_event`.
When the device is connected to the host and configured for the communication, the module will broadcast the :c:enum:`USB_STATE_ACTIVE` state.

HID data handling
=================

The module handles USB HID input report subscriptions.
For the USB legacy stack, all of the USB HID-class instances subscribe to all supported HID reports for the selected protocol.
This occurs when the USB stack reports :c:enum:`USB_DC_CONFIGURED`, and the :c:enum:`USB_STATE_ACTIVE` state is broadcasted.
When leaving the active state, the module will unsubscribe from receiving the HID reports.
For the USB next stack, a separate callback provided by USB HID-class API is used to track HID subscription state.

When the HID report data is transmitted through :c:struct:`hid_report_event`, the module will pass it to the associated USB HID-class instance.
Upon data delivery, :c:struct:`hid_report_sent_event` is submitted by the module.
See :ref:`nrf_desktop_usb_state_sof_synchronization` documentation section for more details about possible HID data flows over USB.

.. note::
    Only one HID input report is submitted by the module to a single instance of HID-class USB device at any given time.
    Subsequent HID input report is provided only after the previous one is sent to USB host.
    Different instances can transmit reports in parallel.

The |usb_state| is a transport for :ref:`nrf_desktop_config_channel` when the channel is enabled.

The module also handles a HID keyboard LED output report received through USB from the connected host.
The module sends the report using :c:struct:`hid_report_event`, that is handled either by :ref:`nrf_desktop_hid_state` (for peripheral) or by the :ref:`nrf_desktop_hid_forward` (for dongle).

nRF54H20 support
================

Due to the characteristics of the nRF54H20 USB Device Controller (UDC), several changes have been made in the USB state module to support the nRF54H20 platform:

* The USB state module creates a separate thread to initialize, enable, and disable the USB stack.
* The module disables the USB stack when the USB cable is disconnected and enables the stack when the cable is connected.

These changes are applicable to the nRF54H20 platform only.
They are necessary to ensure proper USB stack operation on the nRF54H20 platform.

The USB stack cannot be initialized from the system workqueue thread, because it causes a deadlock.
Because of that, a separate thread is used to initialize the USB stack.
For more details, see the :ref:`CONFIG_DESKTOP_USB_INIT_THREAD <config_desktop_app_options>` Kconfig option.
The UDC is powered down whenever the USB cable is disconnected, failing to trigger the necessary callbacks to the USB stack.
It may cause the USB stack to become non-functional.
The USB stack is disabled upon disconnecting the cable to work around this issue.
For more details, see the :ref:`CONFIG_DESKTOP_USB_STACK_NEXT_DISABLE_ON_VBUS_REMOVAL <config_desktop_app_options>` Kconfig option.
