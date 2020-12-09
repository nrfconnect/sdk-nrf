.. _nrf_desktop_hid_state:

HID state module
################

.. contents::
   :local:
   :depth: 2

The |hid_state| is required for generating reports from input data.
It is responsible for the following operations:

* Aggregating data from user input sources.
* Tracking state of the HID report subscriptions.
* Forming the HID reports.
* Transmitting the HID reports to the right subscriber.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_state_start
    :end-before: table_hid_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The |hid_state| module is enabled by selecting :option:`CONFIG_DESKTOP_HID_STATE_ENABLE`.
This module is optional and turned off by default.

Report expiration
=================

With the :option:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION` configuration option, you can set the amount of time after which a key will be considered expired.
The higher the value, the longer the period after which the nRF Desktop application will recall pressed keys when the connection is established.

Queue event size
================

With the :option:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE` configuration option, you can set the number of elements on the queue where the keys are stored before the connection is established.
When a key state changes (it is pressed or released) before the connection is established, an element containing this key's usage is pushed onto the queue.
If there is no space in the queue, the oldest element is released.

Implementation details
**********************

The |hid_state| provides a routing mechanism between sources of input data and transport modules.
This can be associated with:

* Receiving input events from :ref:`nrf_desktop_buttons`, :ref:`nrf_desktop_wheel`, and :ref:`nrf_desktop_motion`.
* Sending out HID reports to :ref:`nrf_desktop_hids` and :ref:`nrf_desktop_usb_state`.

For the routing mechanism to work, the module performs the following operations:

* `Linking input data with the right HID report`_
* `Storing input data before the connection`_
* `Tracking state of transports`_
* `Tracking state of HID report notifications`_
* `Forming HID reports`_


Linking input data with the right HID report
============================================

Out of all available input data types, the following types are collected by the |hid_state|:

* `Relative value data`_ (axes)
* `Absolute value data`_ (buttons)

Both types are stored in the :c:struct:`report_data` structure.

Relative value data
-------------------

This type of input data is related to the pointer coordinates and the wheel rotation.
Both ``motion_event`` and ``wheel_event`` are sources of this type of data.

To indicate a change to this input data, overwrite the value that is already stored.

When either ``motion_event`` or ``wheel_event`` is received, the |hid_state| selects the :c:struct:`report_data` structure associated with the mouse HID report and stores the values at the right position within this structure's ``axes`` member.

.. note::
    The values of axes are stored every time the input data is received, but these values are cleared when a report is connected to the subscriber.
    Consequently, values outside of the connection period are never retained.

Absolute value data
-------------------

This type of input data is related to buttons.
The ``button_event`` is the source of this type of data.

To indicate a change to this input data, overwrite the value that is already stored.

Since keys on the board can be associated to a usage ID, and thus be part of different HID reports, the first step is to identify to which report the key belongs and what usage it represents.
This is done by obtaining the key mapping from the :c:struct:`hid_keymap` structure.
This structure is part of the application configuration files for the specific board and is defined in :file:`hid_keymap_def.h`.

Once the mapping is obtained, the application checks if the report to which the usage belongs is connected:

* If the report is connected, the value is stored at the right position in the ``items`` member of :c:struct:`report_data` associated with the report.
* If the report is not connected, the value is stored in the ``eventq`` event queue member of the same structure.

The difference between these operations is that storing value onto the queue (second case) preserves the order of input events.
See the following section for more information about storing data before the connection.

Storing input data before the connection
========================================

The storing approach before the connection depends on the data type:

* The relative value data is not stored outside of the connection period.
* The absolute value data is stored before the connection.

The reason for this operation is to allow to track key presses that happen right after the device is woken up, but before it is able to connect to the host.

When the device is disconnected and the input event with the absolute value data is received, the data is stored onto the event queue (``eventq``), a member of :c:struct:`report_data` structure.
This queue preserves an order at which input data events are received.

Storing limitations
-------------------

The number of events that can be inserted into the queue is limited by :option:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE`.

Discarding events
    When there is no space for a new input event, the |hid_state| module will try to free space by discarding the oldest event in the queue.
    Events stored in the queue are automatically discarded after the period defined by :option:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION`.

    When discarding an event from the queue, the module checks if the key associated with the event is pressed.
    This is to avoid missing key releases for earlier key presses when the keys from the queue are replayed to the host.
    If a key release is missed, the host could stay with a key that is permanently pressed.
    The discarding mechanism ensures that the host will always receive the correct key sequence.

    .. note::
        The |hid_state| can only discard an event if the event does not overlap any button that was pressed but not released, or if the button itself is pressed.
        The event is released only when the following conditions are met:

        * The associated key is not pressed anymore.
        * Every key that was pressed after the associated key had been pressed is also released.


If there is no space to store the input event in the queue and no old event can be discarded, the entire content of the queue is dropped to ensure the sanity.

Once connection is established, the elements of the queue are replayed one after the other to the host, in a sequence of consecutive HID reports.

Tracking state of transports
============================

The |hid_state| refers collectively to all transports as _subscribers_.

The module tracks the state of the connected Bluetooth LE peers and the state of USB by listening to ``ble_peer_event`` and ``usb_state_event``, respectively.
When the connection to the host is indicated by any of these events, the |hid_state| will create a subscriber associated with the transport.

The subscriber that is associated with USB has priority over any Bluetooth LE peer subscriber.
As a result, when the device connects to the host through USB, all HID reports will be routed to USB.

Tracking state of HID report notifications
==========================================

For each subscriber, the |hid_state| module tracks the state of notifications for each of the available HID reports.
These are tracked in the subscriber's structure :c:struct:`subscriber`.
This structure's member ``state`` is an array of :c:struct:`report_state` structures.
Each element corresponds to one available HID report.

The subscriber connects to the HID reports by submitting ``hid_report_subscription_event``.
Depending on the connection method, this event can be submitted:

* For Bluetooth, when the notification is enabled for a given HID report.
* For USB, when the device is connected to USB.

The :c:struct:`report_state` structure serves the following purposes:

* Tracks the state of the connection.
* Contains the link connecting the object to the right :c:struct:`report_data` structure, from which the data is taken when the HID report is formed.
* Tracks the number of reports of the associated type that were sent to the subscriber.

Forming HID reports
===================

When a HID report is to be sent to the subscriber, the |hid_state| calls the function responsible for the report generation.
The :c:struct:`report_data` structure is passed as an argument to this function.

.. note::
    The HID report formatting function must work according to the HID report descriptor (``hid_report_desc``).
    The source file containing the descriptor is given by :option:`CONFIG_DESKTOP_HID_REPORT_DESC`.

.. |hid_state| replace:: HID state module
