.. _nrf_desktop_hid_provider_system_ctrl:

HID provider system control module
##################################

.. contents::
   :local:
   :depth: 2

The HID provider system control module is responsible for providing system control HID reports.
The module listens to the :c:struct:`button_event` event and communicates with the :ref:`nrf_desktop_hid_state`.
It provides new system control HID reports when requested by the :ref:`nrf_desktop_hid_state`.
It also notifies the :ref:`nrf_desktop_hid_state` when new data is available.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_provider_system_ctrl_start
    :end-before: table_hid_provider_system_ctrl_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

You can enable the module using the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL <config_desktop_app_options>` Kconfig option.
This option is enabled by default if the device supports HID system control reports.
You can substitute the module with a custom HID system control report provider implementation.
Enable the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_ALT <config_desktop_app_options>` Kconfig option to use a custom HID system control report provider.
Make sure to introduce the custom HID system control report provider if you enable this option.

HID keymap
==========

The module uses the :ref:`nrf_desktop_hid_keymap` to map an application-specific key ID to a HID report ID and HID usage ID pair.
The module selects the :ref:`CONFIG_DESKTOP_HID_KEYMAP <config_desktop_app_options>` Kconfig option to enable the utility.
Make sure to configure the HID keymap utility.
See the utility's documentation for details.

Queuing keypresses
==================

The module selects the :ref:`CONFIG_DESKTOP_HID_EVENTQ <config_desktop_app_options>` Kconfig option to enable the :ref:`nrf_desktop_hid_eventq`.
The utility is used to temporarily queue key state changes (presses and releases) before the connection with the HID host is established.
When a key state changes (it is pressed or released) before the connection is established, an element containing this key's usage ID is pushed onto the queue.

Queue size
----------

With the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>` Kconfig option, you can set the number of elements on the queue where the keys are stored before the connection is established.
For backwards compatibility, you can set the default value for this option using the deprecated :ref:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE <config_desktop_app_options>` Kconfig option.
If there is no space in the queue to enqueue a new key state change, the oldest element is released.

Report expiration
-----------------

With the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_KEYPRESS_EXPIRATION <config_desktop_app_options>` Kconfig option, you can set the amount of time after which a queued key will be considered expired.
For backwards compatibility, you can set the default value for this option using the deprecated :ref:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION <config_desktop_app_options>` Kconfig option.
The higher the value, the longer the period from which the nRF Desktop application will recall pressed keys when the connection with HID host is established.

Handling keys state
===================

The module selects the :ref:`CONFIG_DESKTOP_KEYS_STATE <config_desktop_app_options>` Kconfig option to enable the :ref:`nrf_desktop_keys_state`.
The utility is used to track the state of active keys after the connection with the HID host is established.

Implementation details
**********************

On initialization, the module announces its presence by sending the :c:struct:`hid_report_provider_event` event with an appropriate report ID and implementation of callbacks from HID report provider API (:c:struct:`hid_report_provider_api`).
The :ref:`nrf_desktop_hid_state` receives the event and fills it with its own implementation of callbacks from the HID state API (:c:struct:`hid_state_api`).
After that process, the modules can communicate by callbacks.
The module also subscribes to the :c:struct:`button_event` event to get information about the button presses.
The module sends :c:struct:`hid_report_event` events to an appropriate subscriber when it is requested by the :ref:`nrf_desktop_hid_state`.

.. note::
    The HID report formatting function must work according to the HID report descriptor (``hid_report_desc``).
    The source file containing the descriptor is provided by the :ref:`CONFIG_DESKTOP_HID_REPORT_DESC <config_desktop_app_options>` Kconfig option.

Linking input data with the right HID report
============================================

Out of all available input data types, the module collects button events.
The button events are stored in the :c:struct:`report_data` structure.
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

The button data is stored before the connection.

The reason for this operation is to allow to track key presses that happen right after the device is woken up, but before it can connect to the HID host.

When the device is disconnected and the input event with the button data is received, the data is stored onto the :ref:`nrf_desktop_hid_eventq` instance, a member of the :c:struct:`report_data` structure.
This queue preserves an order in which input data events are received.

Storing limitations
-------------------

You can limit the number of events that can be inserted into the queue using the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>` Kconfig option.

Discarding events
------------------

When there is no space for a new input event, the module tries to free space by discarding the oldest event in the queue.
Events stored in the queue are automatically discarded after the period defined by the :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_KEYPRESS_EXPIRATION <config_desktop_app_options>` option.

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
