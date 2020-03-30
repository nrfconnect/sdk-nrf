.. _serialization_entropy_nrf53:

nRF5340: Serialization Entropy
##############################

The Serialization Entropy sample demonstrates how to use the entropy driver in a dual core device such as nRF5340 PDK.

The sample makes use of the entropy driver on the network core of an nRF5340 PDK that generates random data and the :ref:`serialization` that sends the generated data to the application core.

Overview
********


When the sample starts, it displays the generated entropy data in the terminal at an interval of one second.

The entropy data is generated on the network core using the Random Number Genrator (RNG) peripheral.
The :ref:`serialization` uses the `TinyCBOR`_ data format and transmits the data using the default `RPMsg Messaging Protocol`_ (part of `OpenAMP`_) in the transport layer.

The application core uses serialized function calls such as :cpp:func:`entropy_remote_init` and :cpp:func:`entropy_remote_get` to control the entropy driver on the network core.
The :cpp:func:`entropy_remote_init` is used for initializing the entropy and :cpp:func:`entropy_remote_get` is used for obtaining the entropy data.


Network core
============

The network core runs the entropy drivers, which uses the RNG peripheral.
When the network core receives the :cpp:func:`entropy_remote_get` remote function call, the following actions are performed:

   * The network core searches for function decoders in the decoders table and calls it.
   * The network core encodes the response data for the function call and sends the data back to the application core. 

Application core
================

The application core runs a simple application that replaces the entropy driver functions and asynchronous events with the virtual implementation using the :ref:`serialization`.

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

This sample consists of the following sample applications, one each for the application core and the network core:

   * Application core sample: :file:`entropy_nrf53/cpuapp`
   * Network core sample: :file:`entropy_nrf53/cpunet`

Both of these sample applications must be built and flashed to the dual core device before testing.
For details on building samples for a dual core device, see :ref:`ug_nrf5340_building`.

After programming the sample to your board, test it by performing the following steps:

1. Connect the dual core development kit to the computer using a USB cable.
   The development kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the development kit.
#. Observe that the entropy data is displayed periodically in the terminal.


Sample output
=============

The following output is logged on the terminal:

.. code-block:: console

   Entropy sample started[APP Core].                                              
    0x43  0xd1  0xd6  0x52  0x6d  0x22  0x46  0x58  0x8f  0x15                
    0xcf  0xe1  0x1a  0xb5  0xa6  0xdb  0xe5  0xf7  0x7e  0x37   

Dependencies
************


This sample uses the following libraries:

From nrfxlib
  * :ref:`nrfxlib:rp_ser`

From Zephyr
  * :ref:`entropy_interface`
