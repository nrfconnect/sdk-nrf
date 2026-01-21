.. _nrf_desktop_hid_eventq:

HID event queue utility
#######################

.. contents::
   :local:
   :depth: 2

The HID event queue utility can be used by an application module to temporarily queue HID events related to keypresses (button press or release) to handle them later.

Configuration
*************

Make sure that heap size (:kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`) is large enough to handle the worst possible use case.
Data structures used to internally queue HID events are dynamically allocated using the :c:func:`k_malloc` function.
When no longer needed, the structures are freed using the :c:func:`k_free` function.

Use the :option:`CONFIG_DESKTOP_HID_EVENTQ` Kconfig option to enable the utility.
You can use the utility only on HID peripherals (:option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL`).

See Kconfig help for more details.

Using HID event queue
*********************

An application module that handles keypresses internally can use this utility.

Initialization
==============

Initialize a utility instance before use, using the :c:func:`hid_eventq_init` function.
Specify the limit of queued HID events to limit heap usage.

Queuing keypresses
==================

You can use the :c:func:`hid_eventq_keypress_enqueue` function to enqueue a keypress event.
You can use the :c:func:`hid_eventq_keypress_dequeue` function to get a keypress event from the queue.
The keypress events are queued in FIFO (first in first out) manner.
An ID is used to identify the button related to the keypress.
The ID could be, for example, an application-specific identifier of a hardware button or HID usage ID.

You can use the :c:func:`hid_eventq_is_full` and :c:func:`hid_eventq_is_empty` functions, respectively, to determine if the queue is full or empty.

Clearing the queue
==================

You can use the :c:func:`hid_eventq_reset` function to reset the queue.
Resetting the queue results in dropping all of the enqueued keypresses.

You can use the :c:func:`hid_eventq_cleanup` to remove stale keypresses (with timestamp lower than the provided minimal valid timestamp).

API documentation
*****************

Application modules can use the following API of the HID event queue:

| Header file: :file:`applications/nrf_desktop/src/util/hid_eventq.h`
| Source file: :file:`applications/nrf_desktop/src/util/hid_eventq.c`

.. doxygengroup:: hid_eventq
