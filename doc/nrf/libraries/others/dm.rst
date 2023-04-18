.. _mod_dm:

Distance Measurement
####################

.. contents::
   :local:
   :depth: 2

The Distance Measurement module provides an integration of :ref:`nrfxlib:nrf_dm` into |NCS|.

.. note::
   The current implementation is :ref:`experimental <software_maturity>`.

Overview
********

The Distance Measurement module is independent of the communication protocol used.
The :ref:`nrfxlib:mpsl` library is used to access the radio module.
The distance is measured during the allocated timeslot.

Ranging
=======

When a new ranging request arrives, the module checks if a timeslot for the ranging can be scheduled.
It also checks if the request fits in the schedule of rangings.
Rangings are not executed immediately, but are scheduled for future execution, which gives additional flexibility.
New ranging requests may arrive at any point in time.

If the request can be scheduled, the module performs the ranging after the time set in the :kconfig:option:`CONFIG_DM_RANGING_OFFSET_US` option has passed.
The ranging is executed within a timeslot.
After ranging, a callback is called to store or process the measurement data.

Configuration
*************

To use the Distance Measurement module, complete the following steps:

1. Enable the :kconfig:option:`CONFIG_DM_MODULE` Kconfig option.
#. Set the ``ncs,dm-timer`` :ref:`chosen node <zephyr:dt-alias-chosen>` to the TIMER peripheral you wish this module to use.
#. Include :file:`dm.h` in your :file:`main.c` file.
#. Call :c:func:`dm_init()` to initialize the module.
#. Call :c:func:`dm_request_add()` to perform the measurement.

For example, to use the ``TIMER2`` peripheral, set ``ncs,dm-timer`` as follows in a devicetree overlay:

.. code-block:: DTS

   / {
           chosen {
                   ncs,dm-timer = &timer2;
           };
   };

After synchronizing, the nodes that perform the measurement with each other will issue the measurement request.
The callback function :c:func:`data_ready` is called when the measurement data is available.

Configuring synchronization
===========================

To adjust the synchronization of the nodes, change the values of the following options:

* :kconfig:option:`CONFIG_DM_INITIATOR_DELAY_US` - Extra delay of the start of the initiator role for the distance measurement.
  Reducing this value decreases the power consumption, but leads to less successful rangings.
* :kconfig:option:`CONFIG_DM_MIN_TIME_BETWEEN_TIMESLOTS_US` - Minimum time between two timeslots.
  This option should account for processing of the ranging data after the timeslot.

Enabling output pin state change
--------------------------------

It is possible to enable an output pin state change when an event related to this module occurs.
You can use this functionality to determine the synchronization accuracy.
A logic analyzer or oscilloscope are helpful for this purpose.

Enabling the :kconfig:option:`CONFIG_DM_GPIO_DEBUG` option changes the state of the pins when a new measurement request is added and the timeslot is assigned.
To assign the pin numbers, create the appropriate nodes in the devicetree as follows:

.. code-block:: dts

    / {
       dm_gpio {
          compatible = "gpio-leds";
          dm_ranging: dm-ranging {
             gpios = <&gpio0 27 GPIO_ACTIVE_LOW>;
             label = "DM Ranging Pin";
          };
          dm_add_request: dm-add-request {
             gpios = <&gpio0 26 GPIO_ACTIVE_LOW>;
             label = "DM Add request Pin";
          };
       };
    };

Configuring timeslot queue
--------------------------

Use the following options to configure the timeslot queue:

* :kconfig:option:`CONFIG_DM_TIMESLOT_QUEUE_LENGTH` - Maximum number of scheduled timeslots.
* :kconfig:option:`CONFIG_DM_TIMESLOT_QUEUE_COUNT_SAME_PEER` - Maximum number of timeslots with rangings to the same peer.

For optimal performance and scalability, both peers should come to the same decision to range each other.
Otherwise, one of the peers tries to range the other peer that is not listening and therefore wastes power and time during this operation.

If you enable the :kconfig:option:`CONFIG_DM_TIMESLOT_RESCHEDULE` option, the device will try to range the same peer again if the previous ranging was successful.

Defining ranging offset
-----------------------

The option :kconfig:option:`CONFIG_DM_RANGING_OFFSET_US` defines the time between the synchronization (adding a request) and ranging.
Increasing this value allows for more rangings to different nodes, but also increases latency.

API documentation
*****************

| Header file: :file:`include/dm.h`
| Source files: :file:`subsys/dm/`

.. doxygengroup:: dm
   :project: nrf
   :members:
