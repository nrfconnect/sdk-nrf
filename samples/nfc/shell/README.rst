.. _nfc_shell:

NFC: Shell
##########

.. contents::
   :local:
   :depth: 2

This sample demonstrates the NFC transport feature for a shell interface.
It runs the shell interface with the NFC T4T ISO-DEP transport layer.
It uses the :ref:`shell_nfc_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires the polling device with support for raw ISO-DEP (ISO14443-4A) protocol.
The polling device must follow data exchange mechanism described in the :ref:`shell_nfc_readme` library.

Overview
********

This sample presents one of possible ways to run shell through the NFC T4T transport layer.
This is not a common use case for NFC as an NFC tag is a passive device.
However, this feature can be useful, for example, for devices provisioning on the production line.
This sample runs a shell over the NFC transport and implements two shell commands that control **LED 2**.

You can use the following commands:

* The ``led on`` command lits **LED 2**.
* The ``led off`` command dims **LED 2**.

User interface
**************

LED 1:
   Blinks, toggling on/off every second, when the main loop is running.

LED 2:
   Lits or dims when user issues the shell commands that control the LED.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/shell`

.. include:: /includes/build_and_run.txt

.. note::
   |nfc_nfct_driver_note|

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal_ANSI|
#. Reset your development kit.
#. Observe that the sample starts.
#. Touch the NFC antenna with the polling device.
#. Observe that the shell prompt appears on the terminal.
#. Keep the NFC antenna in the polling device field range.
#. Issue the ``led on`` command through the terminal.
#. Observe that the **LED 2** lits.
#. Issue the ``led off`` command through the terminal.
#. Observe that the **LED 2** dims.
#. You can play with other build-in shell commands.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`shell_nfc_readme`

It uses the following Zephyr libraries:

* ``include/zephyr/kernel.h``
* ``zephyr/shell/shell.h``
