.. _nrf_desktop_hid_provider_mouse:

HID provider mouse module
#########################

.. contents::
   :local:
   :depth: 2

The HID provider mouse module is a HID report provider integrated with the :ref:`nrf_desktop_hid_state`.
The module is responsible for providing the HID mouse input report and the HID boot mouse input report.

The module listens to the user input (:c:struct:`button_event`, :c:struct:`wheel_event`, and :c:struct:`motion_event`) and communicates with the :ref:`nrf_desktop_hid_state`.
It provides HID mouse reports when requested by the :ref:`nrf_desktop_hid_state`.
It also notifies the :ref:`nrf_desktop_hid_state` when new data is available.

For details related to the HID report providers integration in the HID state module, see the :ref:`nrf_desktop_hid_state_providing_hid_input_reports` documentation section.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_provider_mouse_start
    :end-before: table_hid_provider_mouse_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

You can enable the default implementation of the HID provider using the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_MOUSE` Kconfig option.
This option is enabled by default if the device uses HID provider events (:option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_EVENT`) and supports HID mouse reports (:option:`CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT`).
The default implementation of the HID provider uses a predefined format of HID reports, which is aligned with the default HID report map in the common configuration (:option:`CONFIG_DESKTOP_HID_REPORT_DESC`).
The module also provides HID boot mouse input report if it is supported (:option:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE`).

Alternatively, you can substitute the module with a custom HID mouse report provider implementation.
Using the custom provider allows you to modify the sources of user input and the HID report format.
Enable the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_MOUSE_ALT` Kconfig option to use a custom HID mouse report provider.
The option disables the default HID mouse report provider.
Make sure to introduce the custom HID mouse report provider if you enable this option.

Default implementation
======================

If you use the default implementation, you only need to configure the HID keymap for the module.

HID keymap
----------

The module uses the :ref:`nrf_desktop_hid_keymap` to map an application-specific key ID to a HID report ID and HID usage ID pair.
The module selects the :option:`CONFIG_DESKTOP_HID_KEYMAP` Kconfig option to enable the utility.
Make sure to configure the HID keymap utility.
See the utility's documentation for details.

Implementation details
**********************

The module is used by :ref:`nrf_desktop_hid_state` as a HID input report provider for the HID mouse input report and HID boot mouse input report.
The module registers two separate HID report providers to handle both input reports.
On initialization, the module submits the events of type :c:struct:`hid_report_provider_event` to establish two-way callbacks between the |hid_state| and the HID report providers.

Handling user input
===================

The module handles the following types of user input:

* `Relative value data`_ (axes)
  This type of input data is related to the pointer coordinates and the wheel rotation.
  Both :c:struct:`motion_event` and :c:struct:`wheel_event` are sources of this type of data.
* `Absolute value data`_ (buttons)
  This type of input data is related to the state of pressed buttons.
  The :c:struct:`button_event` is the source of this type of data.

Both input types are stored internally in the ``report_data`` structure.

Relative value data
-------------------

The relative value data is stored within the ``report_data`` structure's ``axes`` member.
There are separate fields for wheel, mouse motion over the X axis, and mouse motion over the Y axis.

The values of axes are stored whenever the input data is received, but these values are cleared when HID report subscription is enabled.
Consequently, values outside of the HID subscription period are never retained.

HID mouse pipeline
~~~~~~~~~~~~~~~~~~

The nRF Desktop application synchronizes motion sensor sampling with sending HID mouse input reports to the HID host.
The HID provider mouse module tracks the number of HID reports in flight and maintains a HID report pipeline to ensure proper HID report flow:

* If the number of HID reports in flight is lower than the pipeline size, the module provides additional HID reports to generate the HID report pipeline on user input.
* If the number of HID reports in flight is greater than or equal to the pipeline size, the module buffers user input internally and delays providing subsequent HID input reports until previously submitted reports are sent to the HID host.
  If the motion source is active, then after a HID input report handled by the module is sent, the module waits for a subsequent :c:struct:`motion_event` before submitting a subsequent HID input report.
  This is done to ensure that the recent value of motion will be included in the subsequent HID input report.

See the :ref:`nrf_desktop_hid_mouse_report_handling` section for an overview of handling HID mouse input reports in the nRF Desktop.
The section focuses on interactions between application modules.

Absolute value data
-------------------

After an application-specific key ID (:c:member:`button_event.key_id`) is mapped to the HID mouse input report ID and related HID usage ID, the HID usage ID is handled by the provider.
The module remembers HID usage IDs of all the currently pressed keys as a bitmask.
The bitmask is then included as part of the HID mouse input report or HID boot mouse input report.
Because of the fact that HID usage IDs are stored as a bitmask, the module does not support handling multiple hardware buttons mapped to the same HID usage ID.

The module ignores keypresses that happen before HID report subscription is enabled and clears the HID usage ID bitmask when HID report subscription is disabled.
As a result, keypresses outside of the HID subscription period are never retained.
