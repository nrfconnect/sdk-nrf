.. _channel_sounding_ipt_reflector:

Bluetooth: Channel Sounding Reflector with Inline PCT Transfer
##############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use Channel Sounding with the inline PCT transfer feature on a reflector device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a device running the :ref:`channel_sounding_ipt_initiator` sample.

Overview
********

The sample demonstrates a basic Bluetooth® Low Energy Peripheral role that configures the Channel Sounding reflector role and supports inline PCT transfer (IPT).
It advertises using its device name, which the :ref:`channel_sounding_ipt_initiator` sample scans for.
Once connected, the reflector enables its CS reflector role and waits for the initiator to create a CS configuration and start procedures.

Unlike the :ref:`channel_sounding_ras_reflector` sample, this sample does not include the GATT Ranging Service (RAS).
Instead, the initiator reads the CS IQ data directly from the controller using IPT, reducing the overhead of the GATT-based data transfer path.

User interface
**************

The sample does not require user input.
The first LED on the development kit will be lit when a connection has been established.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/channel_sounding/ipt_reflector`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it together with a device programmed with the :ref:`channel_sounding_ipt_initiator` sample.

1. |connect_terminal_specific|
#. Reset both kits.
#. The reflector starts advertising and the initiator scans for ``Nordic CS Reflector``.
   Once the initiator connects, check for information similar to the following in the reflector terminal::

      I: Connected to xx.xx.xx.xx.xx.xx (random) (err 0x00)
      I: CS capability exchange completed.
      I: CS config creation complete.
      I: CS security enabled.
      I: CS procedures enabled.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`:

  * :file:`include/logging/log.h`

* :file:`include/zephyr/types.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/uuid.h`
* :file:`include/bluetooth/cs.h`
