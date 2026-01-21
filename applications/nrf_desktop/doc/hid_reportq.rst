.. _nrf_desktop_hid_reportq:

HID report queue utility
########################

.. contents::
   :local:
   :depth: 2

A HID subscriber (for example, a USB HID class instance handled by :ref:`nrf_desktop_usb_state`) limits the maximum number of HID input reports that can be handled simultaneously.
Enqueuing HID input reports locally at the source is necessary to prevent HID report drops when receiving more HID reports than can be handled by the subscriber.
The HID report queue utility can be used by an application module to simplify enqueuing HID input reports received from connected HID peripherals before providing them to HID subscriber.

Configuration
*************

Make sure that heap size (:kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`) is large enough to handle the worst possible use case.
Data structures used to enqueue HID reports are dynamically allocated using the :c:func:`k_malloc` function.
When no longer needed, the structures are freed using the :c:func:`k_free` function.

Use the :option:`CONFIG_DESKTOP_HID_REPORTQ` Kconfig option to enable the utility.
You can use the utility only on HID dongles (:option:`CONFIG_DESKTOP_ROLE_HID_DONGLE`).

You can configure the following properties:

* Maximum number of enqueued HID reports (:option:`CONFIG_DESKTOP_HID_REPORTQ_MAX_ENQUEUED_REPORTS`)
* Number of supported HID report queues (:option:`CONFIG_DESKTOP_HID_REPORTQ_QUEUE_COUNT`)

See Kconfig help for more details.

Using HID report queue
**********************

You can use the utility in a HID dongle's application module that handles receiving HID input reports from HID peripherals.
The module uses the utility as a middleware between a connected HID peripheral and a HID subscriber.
The module passes the received HID input report to the HID report queue utility.
The HID report queue utility submits the :c:struct:`hid_report_event` to pass the HID input report to the HID subscriber.
The following sections describe how to integrate HID report queue APIs in an application module.

The :ref:`nrf_desktop_hid_forward` is an example of an application module that uses the HID report queue utility.
Refer to the module's implementation (:file:`src/modules/hid_forward.c`) for an example of HID report queue integration.

Allocation
==========

A HID report queue instance must be allocated with the :c:func:`hid_reportq_alloc` function before it is used.
A separate HID report queue object needs to be allocated for each HID report subscriber.
During allocation, the caller specifies the HID subscriber identifier.
The :c:func:`hid_reportq_get_sub_id` function can be used to access the identifier later.
After the HID report queue object is no longer used, it should be freed using the :c:func:`hid_reportq_free` function.

HID subscriptions
=================

A HID report queue instance tracks HID input report subscriptions enabled by a HID subscriber.
You can use the :c:func:`hid_reportq_subscribe` and :c:func:`hid_reportq_unsubscribe` functions to update the state of enabled subscriptions.
The functions should be called while handling a :c:struct:`hid_report_subscription_event` in the module using the HID reportq utility.
You can use the :c:func:`hid_reportq_is_subscribed` function to check if the subscription is enabled for the specified HID input report.

Enqueuing HID input reports
===========================

If an application module uses the HID report queue instance to locally enqueue HID input reports for a given HID subscriber, every HID report intended for the subscriber should go through the HID report queue.
The application module can call the :c:func:`hid_reportq_report_add` function for a received HID input report to pass the report to the HID report queue utility.
The function allocates a :c:struct:`hid_report_event` for the received HID input report.
If a HID subscriber can handle the :c:struct:`hid_report_event`, the event is instantly passed to the subscriber.
Otherwise, the event is enqueued and will be submitted later.

When a HID subscriber (for example, a USB HID class instance) delivers a HID input report to the HID host (on :c:struct:`hid_report_sent_event`), the :c:func:`hid_reportq_report_sent` API needs to be called to notify the HID report queue.
This allows the queue to track the state of HID reports provided to the HID subscriber.
If the HID report queue contains an enqueued report, the queue object will instantly submit the subsequent :c:struct:`hid_report_event`.
The enqueued report to be sent is chosen in the round-robin fashion from HID report IDs with enabled subscription.
The report with the next report ID will be sent if available.
If not available, the next report IDs will be checked until a report is found or until the utility detects that there are no more enqueued reports.

API documentation
*****************

Application modules can use the following API of the HID report queue:

| Header file: :file:`applications/nrf_desktop/src/util/hid_reportq.h`
| Source file: :file:`applications/nrf_desktop/src/util/hid_reportq.c`

.. doxygengroup:: hid_reportq
