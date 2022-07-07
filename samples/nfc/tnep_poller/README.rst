.. _nfc_tnep_poller:

NFC: TNEP poller
################

.. contents::
   :local:
   :depth: 2

The NFC TNEP poller sample demonstrates how to use the :ref:`tnep_poller_readme` library to exchange data using an NFC polling device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample has the following additional requirements:

* NFC Reader ST25R3911B Nucleo expansion board (X-NUCLEO-NFC05A1)
* NFC Type 4 Tag TNEP device

Overview
********

The sample interacts with the NFC Type 4 Tag.

The sample reads the NFC Type 4 Tag and looks for the TNEP initial message.
After finding it, the first service from the message is selected and the poller attempts to exchange data.
Next, the service is deselected.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/tnep_poller`

.. include:: /includes/build_and_run.txt

.. note::
   |nfc_nfct_driver_note|

Testing
=======

After programming the sample to your development kit, you can test it with an NFC-A Tag device that supports NFC's TNEP.
Complete the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Put the NFC Tag device antenna in the range of the NFC polling device.
   The NFC polling device selects the first service and exchanges basic data with it.
   After that, the service is deselected.
#. Observe the output in the terminal.

Dependencies
************

This sample uses the following |NCS| drivers:

* :ref:`st25r3911b_nfc_readme`

It uses the following |NCS| libraries:

* :ref:`tnep_poller_readme`
* :ref:`nfc_ndef_parser_readme`
* :ref:`nfc_t4t_apdu_readme`
* :ref:`nfc_t4t_isodep_readme`
* :ref:`nfc_t4t_hl_procedure_readme`
* :ref:`nfc_t4t_ndef_file_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* ``include/zephyr.h``
