.. _serialization_entropy_nrf53:

Serialization: Entropy nRF5340
##################################

The Entropy sample demonstrates how to use the Entropy driver on a dual core device.
It uses the Entropy driver on the Network Core which has the possibility to generate random number and using the
Remote Procedure Serialization library sending it to the Application core.
The application core uses the serialized function call to control the Entropy driver on the Network core like
:cpp:func:`entropy_remote_init` for initializing the entropy and :cpp:func:`entropy_remote_get` for getting entropy data.


Overview
********

When sample started, it displays the generated entropy data on the Application core terminal with one second interval.
The Entropy data is generated on the Network core using RNG peripheral.
The Remote Procedure Serialization uses `TinyCBOR`_ data format and transmit it using default RPMsg (part of `OpenAMP`_) transport layer.

Network core
============

The Network core runs the Entropy drivers which uses the RNG periphery. When Network core received the Remote function call, it
look for function decoders in decoders table and call it after that the Network core encode function response data and sends it back.

Application core
================

The application core runs a simple application, where the Entropy drivers functions and asynchronous events are replaced by virtual implementation using Remote Procedure Serialization library.
For the needs of the Entropy driver, this application uses only :cpp:func:`entropy_remote_init` to initialize the Network core  Entropy driver and :cpp:func:`entropy_remote_get` to gets entropy data.

Requirements
************

* The following development board:

  * |nRF5340DK|


Building and running
********************
.. |sample path| replace:: :file:`samples/serialization/entropy_nrf53`

.. include:: /includes/build_and_run.txt

.. _entropy_nrf53_testing:

Testing
=======

This sample consists of two sample applications, one for the network core, and one for the application core:

   * Application core sample: :file:`entropy_nrf53/cpuapp`
   * Network core sample: :file:`entropy_nrf53/cpunet`

Both of these sample applications must be built and flashed before testing.
For details on building samples for a dual core device, see :ref:`ug_nrf5340_building`.

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the computer using a USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the board.
#. Entropy data should be printed periodically on the terminal.
