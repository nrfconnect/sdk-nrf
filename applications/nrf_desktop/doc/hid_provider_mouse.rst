.. _nrf_desktop_hid_provider_mouse:

HID provider mouse module
#########################

.. contents::
   :local:
   :depth: 2

The HID provider mouse module is responsible for providing mouse HID reports.
The module listens to the :c:struct:`button_event`, :c:struct:`wheel_event` and :c:struct:`motion_event` events and communicates with the :ref:`nrf_desktop_hid_state`.
It provides new mouse HID reports when requested by the :ref:`nrf_desktop_hid_state`.
It also notifies the :ref:`nrf_desktop_hid_state` when new data is available.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_provider_mouse_start
    :end-before: table_hid_provider_mouse_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

You can enable the module using the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_MOUSE <config_desktop_app_options>` Kconfig option.
This option is enabled by default if the device supports HID mouse reports.
You can substitute the module with a custom HID mouse report provider implementation.
Enable the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_MOUSE_ALT <config_desktop_app_options>` Kconfig option to use a custom HID mouse report provider.
Make sure to introduce the custom HID mouse report provider if you enable this option.

HID keymap
==========

The module uses the :ref:`nrf_desktop_hid_keymap` to map an application-specific key ID to a HID report ID and HID usage ID pair.
The module selects the :ref:`CONFIG_DESKTOP_HID_KEYMAP <config_desktop_app_options>` Kconfig option to enable the utility.
Make sure to configure the HID keymap utility.
See the utility's documentation for details.

Implementation details
**********************

On initialization, the module announces its presence by sending a :c:struct:`hid_report_provider_event` event with appropriate report ID and implementation of callbacks from the HID report provider API (:c:struct:`hid_report_provider_api`).
Separate :c:struct:`hid_report_provider_event` events are sent for each report ID.
The module supports mouse reports from the report and boot protocols.
You can enable the boot protocol support using the :ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>` Kconfig option.
The :ref:`nrf_desktop_hid_state` receives the events and fills them with its own implementation of callbacks from the HID state API (:c:struct:`hid_state_api`).
After that process, the modules can communicate by callbacks.
The module also subscribes to the :c:struct:`button_event`, :c:struct:`wheel_event` and :c:struct:`motion_event` events to get information about the button presses, wheel scrolls, and motion events.
The module sends :c:struct:`hid_report_event` events to an appropriate subscriber when it is requested by the :ref:`nrf_desktop_hid_state`.

.. note::
    The HID report formatting function must work according to the HID report descriptor (``hid_report_desc``).
    The source file containing the descriptor is provided by the :ref:`CONFIG_DESKTOP_HID_REPORT_DESC <config_desktop_app_options>` Kconfig option.

Linking input data with the right HID report
============================================

Out of all available input data types, the following types are collected by this module:

* `Relative value data`_ (axes)
* `Absolute value data`_ (buttons)

Both types are stored in the :c:struct:`report_data` structure.

Relative value data
-------------------

This type of input data is related to the pointer coordinates and the wheel rotation.
Both ``motion_event`` and ``wheel_event`` are sources of this type of data.

When either ``motion_event`` or ``wheel_event`` is received, the module selects the :c:struct:`report_data` structure associated with the mouse HID report and stores the values at the right position within this structure's ``axes`` member.

.. note::
    The values of axes are stored every time the input data is received, but these values are cleared when a report is connected to the subscriber.
    Consequently, values outside of the connection period are never retained.

Absolute value data
-------------------

This type of input data is related to buttons.
The ``button_event`` is the source of this type of data.

To indicate a change to this input data, the module overwrites the value that is already stored.

Since keys on the board can be associated with a HID usage ID and thus be part of different HID reports, the first step is to identify if the key belongs to a HID report that is provided by this module.
This is done by obtaining the key mapping from the :ref:`nrf_desktop_hid_keymap`.

Once the mapping is obtained, the application checks if the report to which the usage belongs is connected:

* If the report is connected and the :ref:`nrf_desktop_hid_eventq` instance is empty, the module stores the report and calls the ``trigger_report_send`` callback from the :c:struct:`hid_state_api` to notify the :ref:`nrf_desktop_hid_state` about the new data.
* If the report is not connected or the :ref:`nrf_desktop_hid_eventq` instance is not empty, the value is enqueued in the :ref:`nrf_desktop_hid_eventq` instance.

The difference between these operations is that storing a value onto the queue (second case) preserves the order of input events.
See the following section for more information about storing data before the connection.

Storing input data before the connection
========================================

The storing approach before the connection depends on the data type:

* The relative value data is not stored outside of the connection period.
* The absolute value data is stored before the connection.

The reason for this operation is to allow to track key presses that happen right after the device is woken up, but before it can connect to the HID host.

When the device is disconnected and the input event with the button data is received, the data is stored onto the :ref:`nrf_desktop_hid_eventq` instance, a member of the :c:struct:`report_data` structure.
This queue preserves an order in which input data events are received.

Discarding events
------------------

When there is no space for a new input event, the module tries to free space by discarding the oldest event in the queue.

When discarding an event from the queue, the module checks if the key associated with the event is pressed.
This is to avoid missing key releases for earlier key presses when the keys from the queue are replayed to the host.
If a key release is missed, the host could stay with a key that is permanently pressed.
The discarding mechanism ensures that the host will always receive the correct key sequence.

.. note::
    The module can only discard an event if the event does not overlap any button that was pressed but not released, or if the button itself is pressed.
    The event is released only when the following conditions are met:

    * The associated key is not pressed anymore.
    * Every key that was pressed after the associated key had been pressed is also released.

If there is no space to store the input event in the queue and no old event can be discarded, the entire content of the queue is dropped to ensure the sanity.

Once the connection is established, the elements of the queue are replayed one after the other to the host, in a sequence of consecutive HID reports.
