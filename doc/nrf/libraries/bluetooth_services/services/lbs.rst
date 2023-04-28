.. _lbs_readme:

LED Button Service (LBS)
########################

.. contents::
   :local:
   :depth: 2

The GATT LED Button Service (LBS) is a custom service that receives information about the state of an LED and sends notifications when a button changes its state.

The LBS Service is used in the :ref:`peripheral_lbs` sample.

Service UUID
************

The 128-bit vendor-specific service UUID is ``00001523-1212-EFDE-1523-785FEABCD123``.

Characteristics
***************

This service has two characteristics.

Button Characteristic (``00001524-1212-EFDE-1523-785FEABCD123``)
================================================================

Notify:
    Enable notifications for the Button Characteristic to receive button data from the application.

Read:
    Read button data from the application.

LED Characteristic (``00001525-1212-EFDE-1523-785FEABCD123``)
=============================================================

Write:
    Write data to the LED Characteristic to change the LED state.

API documentation
*****************

| Header file: :file:`include/lbs.h`
| Source file: :file:`subsys/bluetooth/services/lbs.c`

.. doxygengroup:: bt_lbs
   :project: nrf
   :members:
