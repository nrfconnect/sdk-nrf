.. _nrf_desktop_hid_provider_keyboard:

HID provider keyboard module
############################

.. contents::
   :local:
   :depth: 2

The HID provider keyboard module is a HID report provider integrated with the :ref:`nrf_desktop_hid_state`.
The module is responsible for providing the HID keyboard input report and the HID boot keyboard input report.

The module listens to the user input (:c:struct:`button_event`) and communicates with the :ref:`nrf_desktop_hid_state`.
It provides HID keyboard reports when requested by the :ref:`nrf_desktop_hid_state`.
It also notifies the :ref:`nrf_desktop_hid_state` when new data is available.

For details related to the HID report providers integration in the HID state module, see the :ref:`nrf_desktop_hid_state_providing_hid_input_reports` documentation section.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_provider_keyboard_start
    :end-before: table_hid_provider_keyboard_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

You can enable the default implementation of the HID provider using the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD` Kconfig option.
This option is enabled by default if the device uses HID provider events (:option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_EVENT`) and supports HID keyboard reports (:option:`CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT`).
The default implementation of the HID provider uses a predefined format of HID reports, which is aligned with the default HID report map in the common configuration (:option:`CONFIG_DESKTOP_HID_REPORT_DESC`).
The module also provides HID boot keyboard input report if it is supported (:option:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD`).

Alternatively, you can substitute the module with a custom HID keyboard report provider implementation.
Using the custom provider allows you to modify the sources of user input and the HID report format.
Enable the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_ALT` Kconfig option to use a custom HID keyboard report provider.
The option disables the default HID keyboard report provider.
Make sure to introduce the custom HID keyboard report provider if you enable this option.

Default implementation
======================

The module relies on keypresses (:c:struct:`button_event`) as the only source of user input.
The module needs to perform the following tasks:

* Identify keypresses related to the handled HID input reports.
* Queue a sequence of the user's key presses and releases that happens before connecting to a HID host.
  The reason for this operation is to allow tracking key presses that happen right after the device is woken up but before it can connect to the HID host.
  The sequence of keypresses is replayed once a connection with a HID host is established.
* Maintain the state of pressed keys.
  You need this to report the current state of pressed keys while a connection with the HID host is maintained.

The module uses a set of application-specific utilities for that purpose.
See the following sections for the configuration details of the used application-specific utilities.

HID keymap
----------

Since the keys on the board can be associated with a HID usage ID and thus be part of different HID reports, the first step is to identify if the key belongs to a HID report that is provided by this module.
This is done by obtaining the key mapping from the :ref:`nrf_desktop_hid_keymap`.
The module selects the :option:`CONFIG_DESKTOP_HID_KEYMAP` Kconfig option to enable the utility.
Make sure to configure the HID keymap utility.
See the utility's documentation for details.

HID eventq
----------

The :ref:`nrf_desktop_hid_eventq` is used to temporarily enqueue key state changes before connection with the HID host is established.
When a key is pressed or released before the connection is established, an element containing this key's usage ID is pushed onto the queue.
The sequence of keypresses is replayed once the connection with a HID host is established.
This ensures that all of the keypresses are replayed in order.
The module selects the :option:`CONFIG_DESKTOP_HID_EVENTQ` Kconfig option to enable the utility.

Queue size
~~~~~~~~~~

With the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_EVENT_QUEUE_SIZE` Kconfig option, you can set the number of elements on the queue where the keys are stored before the connection is established.
For backwards compatibility, you can set the default value for this option using the deprecated :option:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE` Kconfig option.
If there is no space in the queue to enqueue a new key state change, the oldest element is released.

Report expiration
~~~~~~~~~~~~~~~~~

With the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_KEYPRESS_EXPIRATION` Kconfig option, you can set the amount of time after which a queued key will be considered expired.
For backwards compatibility, you can set the default value for this option using the deprecated :option:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION` Kconfig option.
The higher the value, the longer the period from which the nRF Desktop application will recall pressed keys when the connection with HID host is established.

Keys state
----------

The :ref:`nrf_desktop_keys_state` is used to track the state of active keys after the connection with the HID host is established.
The module selects the :option:`CONFIG_DESKTOP_KEYS_STATE` Kconfig option to enable the utility.

Implementation details
**********************

The module is used by :ref:`nrf_desktop_hid_state` as a HID input report provider for the HID keyboard input report and HID boot keyboard input report.
The module registers two separate HID report providers to handle both input reports.
On initialization, the module submits the events of type :c:struct:`hid_report_provider_event` to establish two-way callbacks between the |hid_state| and the HID report providers.

Handling keypresses
===================

After an application-specific key ID (:c:member:`button_event.key_id`) is mapped to the HID keyboard input report ID and related HID usage ID, the HID usage ID is handled by the provider.
Before a connection with the HID host is established, the provider enqueues the HID usage ID and keypress state (press or release) in the HID event queue.
Once the connection is established, the elements of the queue are replayed one after the other to the host, in a sequence of consecutive HID reports.
On the HID state request, the module pulls an element from the queue, updates the tracked state of pressed keys, and submits a :c:struct:`hid_report_event` to provide the HID input report to the HID subscriber.
The subsequent requests lead to providing subsequent keypresses as HID report events.
All key state changes still go through the HID event queue until the queue is empty.

Once the queue is empty, a key state change results in an instant update of the tracked state of pressed keys.
If the state of pressed keys changes, the module calls the :c:member:`hid_state_api.trigger_report_send` callback to notify the :ref:`nrf_desktop_hid_state` about the new data.
The module also remembers that the HID subscriber needs to be updated.

Discarding queued events
------------------------

While key state changes go through the event queue and there is no space for a new input event, the module tries to free space by discarding the oldest event in the queue.
Events stored in the queue are automatically discarded after the period defined by the :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_KEYPRESS_EXPIRATION` option.

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
