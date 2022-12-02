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
