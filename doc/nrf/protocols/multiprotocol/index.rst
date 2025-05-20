.. _ug_multiprotocol_support:

Multiprotocol support
#####################

.. contents::
   :local:
   :depth: 2

The :ref:`Thread <ug_thread>` and :ref:`Zigbee <ug_zigbee>` protocols support running another protocol in parallel.

This is achieved with the :ref:`Multi-Protocol Service Layer (MPSL) <nrfxlib:mpsl>` library, which provides services for multiprotocol applications and allows the 802.15.4 radio driver to negotiate for transmission timeslots with the radio controller.
In |NCS|, MPSL support is ensured by :ref:`nrfxlib:softdevice_controller`, an implementation of the :ref:`ug_ble_controller`.
For more information about how the SoftDevice Controller and MPSL work together, read the :ref:`SoftDevice Controller's documentation <softdevice_controller_readme>`.

The multiprotocol solution available for Thread and Zigbee is dynamic.
This means that it allows for running several radio protocols simultaneously without the time-expensive uninitialization and initialization in-between the switching.
Switching between protocols requires only a reinitialization of the radio peripheral, since protocols may operate on different frequencies and modulations.

Multiprotocol limitations in application development
****************************************************

Given the nature of the dynamic multiprotocol solution, take into account the following considerations when developing your Thread or Zigbee application:

* Any kind of application or Thread/Zigbee stack logic can be interrupted by the Bluetooth LE activity.
  This is the primary difference between a Thread/Zigbee-only application and a multiprotocol one.
  As a result, it is acceptable that the node can sometimes miss a frame and not respond with a MAC-level acknowledgment.
* Bluetooth® LE scanning requires a lot of time to process Bluetooth LE traffic and thus blocks 802.15.4 traffic.
  When the scanning is used, it is recommended to use the Sleepy End Device role for Thread or Zigbee applications.
* Be aware that flash operations require more time to complete.
  A flash operation may be performed only in a free timeslot, so it has to wait until the current timeslot finishes (it may be a 802.15.4 timeslot or a Bluetooth LE one).
  Moreover, the Thread or Zigbee stacks cannot operate while the flash operation is being performed.
  Avoid an application design that involves a continuous series of small flash operations.
  The time the stack will be blocked by such a series of operations can be estimated using the following formula:

  * ``((smallest_timeslot_time + op_flash_write_time) * n_of_flash_ops)``

  In this formula:

  * ``smallest_timeslot_time`` is the smallest timeslot that the radio requests. In the default configuration, it is 6.4 ms.
  * ``op_flash_write_time`` is the time required to perform a flash operation.
  * ``n_of_flash_ops`` is the number of flash operations to be performed in a sequence.

* The Zigbee stack has a limited TX queue.
  This queue can get full in case of heavy application traffic and Bluetooth LE activity.
  The application logic must take this into account by providing a callback for all crucial transmissions and handle the ``MAC_TRANSACTION_OVERFLOW (== 241)`` status code.
* The most time-critical part of the Zigbee stack operation is the Base Device Behavior (BDB) commissioning process.
  Postpone time-consuming operations until the ``ZB_BDB_SIGNAL_DEVICE_FIRST_START`` or ``ZB_BDB_SIGNAL_DEVICE_REBOOT`` signal is received.

.. _multiprotocol_bt_thread:

Bluetooth LE and Thread coexistence
***********************************

The Thread protocol has a lower priority than Bluetooth LE.
When Bluetooth LE consumes a significant amount of radio time, such as during Bluetooth LE central start scanning or to maintain a large number of connections, this can lead to insufficient time left for Thread to access the radio.
To maintain effective Thread communication, it is essential to reduce the radio time for Bluetooth LE.
This can be achieved by adjusting the scan and connection parameters.

Bluetooth LE scan parameters
============================

The key parameters for scanning are:

- *Scan interval* - The time between consecutive scans.
- *Scan window* - The amount of time within the scan interval that is dedicated to actual scanning.

By reducing the scan window for a given scan interval, the actual radio time used by Bluetooth LE decreases, thereby freeing up more time for Thread.
Similarly, increasing the scan interval for a set scan window also frees up more radio time.
Note that this is not purely about the overall percentage of time used by Bluetooth LE.

For example, while both a 20 ms scan window with a 60 ms interval and a 100 ms window with a 300 ms interval consume the same 33% percentage of time, they do not have the same effect.
The key difference lies in how long Bluetooth LE consistently occupies the radio during active scanning.
If the scan window is too long, the likelihood of Thread transmissions clashing with Bluetooth LE activity rises.
This can lead to collisions, and can result in communication delays or even complete communication breakdown.

.. note::
   If the scan time is too short, advertising packets from the peripherals might be skipped.

Bluetooth LE connection parameters
==================================

The key parameters for connection are:

- *Connection interval* - Time between successive wake-ups of the Bluetooth LE devices for communication.
- *Event space time* - The time between consecutive events or connections with paired Bluetooth LE devices.

These parameters are chosen for a single device, but the total time occupied by Bluetooth LE communication must be multiplied by the number of connected devices.
As a result, when multiple Bluetooth LE peripherals are connected, the remaining time for Thread could be insufficient.
To increase the available time for Thread, you can, for instance, increase the connection interval.
It will decrease the communication throughput, but will free up more time for Thread.

Another option is to reduce the event space time.
This time can be optionally used when the peripheral device has a big amount of data to send and it exceeds typical width of radio slot.
Reducing the event space time is an ideal option when there is little data to transmit over Bluetooth LE because it minimizes the idle time between events.
This leaves more time for Thread communication across the entire timeline.

Example parameter values
========================

It is important to note that there is no universally optimal set of parameters for every scenario.
The selection of these parameters should be based on specific assumptions, as different values will be more suitable depending on the expected Bluetooth LE throughput,the device's responsiveness, and the anticipated Thread radio link quality.
The following values assume minimal data transmission from the device and aim to maintain a Thread radio retransmission rate below 10%.

- *Scan interval* equal to 100 ms.
- *Scan window* equal to 25 ms.
- *Connection interval* equal to 100 ms.
- *Event space time* equal to 5.5 ms.
