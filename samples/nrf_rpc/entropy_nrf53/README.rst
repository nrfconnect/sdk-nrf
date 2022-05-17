.. _nrf_rpc_entropy_nrf53:

nRF5340: nRF RPC Entropy
########################

.. contents::
   :local:
   :depth: 2

The nRF RPC Entropy sample demonstrates how to use the entropy driver in a dual core device such as nRF5340 DK.

The sample uses the entropy driver on the network core of an nRF5340 DK that generates random data, and the :ref:`nrfxlib:nrf_rpc` that sends the generated data to the application core using `Remote Procedure Calls (RPC)`_.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The entropy data is generated on the network core using the Random Number Generator (RNG) peripheral.
The :ref:`nrfxlib:nrf_rpc` uses the `zcbor`_ to encode data, into the CBOR format, and transmits the data using the default `RPMsg Messaging Protocol`_ (part of `OpenAMP`_) in the transport layer.

The application core uses serialized function calls such as :c:func:`entropy_remote_init` and :c:func:`entropy_remote_get` to control the entropy driver on the network core as follows:

* The :c:func:`entropy_remote_init` function is used for initializing the entropy.
* The :c:func:`entropy_remote_get` function is used for obtaining the entropy data.

When the sample starts, it displays the generated entropy data in the terminal at an interval of two seconds.

Network core
============

The network core runs the entropy drivers that use the RNG peripheral.
When the network core receives the :c:func:`entropy_remote_get` remote function call, the following operations are performed:

* The network core searches for function decoders in the decoders table and calls them.
* The network core encodes the response data for the function call and sends the data back to the application core.

Application core
================

The application core runs a simple application that replaces the entropy driver functions and asynchronous events with the virtual implementation using the :ref:`nrfxlib:nrf_rpc`.

Building and running
********************
.. |sample path| replace:: :file:`samples/nrf_rpc/entropy_nrf53`

.. include:: /includes/build_and_run.txt

.. _entropy_nrf53_testing:

Testing
=======

This sample consists of the following sample applications:

* Application core sample: :file:`entropy_nrf53/cpuapp`
* Network core sample: :file:`entropy_nrf53/cpunet`

Build and program both of these sample applications to the dual core device before testing.
For details on building samples for a dual core device, see :ref:`ug_nrf5340_building`.

After programming the sample applications to your development kit, complete the following steps to test this sample:

1. Connect the dual core development kit to the computer using a USB cable.
   The development kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the development kit.
#. Observe that the entropy data is displayed periodically in the terminal.

Sample output
=============

The following output is displayed in the terminal:

.. code-block:: console

   Entropy sample started[APP Core].
    0x43  0xd1  0xd6  0x52  0x6d  0x22  0x46  0x58  0x8f  0x15
    0xcf  0xe1  0x1a  0xb5  0xa6  0xdb  0xe5  0xf7  0x7e  0x37

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`

It also uses the following Zephyr library:

 * :ref:`zephyr:entropy_api`
