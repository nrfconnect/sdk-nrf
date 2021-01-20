.. _nfc_tnep_poller:

NFC: TNEP poller
################

.. contents::
   :local:
   :depth: 2

The NFC TNEP poller sample demonstrates how to use the :ref:`tnep_poller_readme` library to exchange data using an NFC polling device.

Overview
********

The sample can interact with the NFC Type 4 Tag.

Initially, the sample reads the NFC Type 4 Tag and looks for the TNEP initial message.
After finding it, the first service from the message is selected and the poller attempts to exchange data.
Next, the service is deselected.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833

The sample has the following additional requirements:

* NFC Reader ST25R3911B Nucleo expansion board (X-NUCLEO-NFC05A1)
* NFC Type 4 Tag TNEP device

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/tnep_poller`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it with an NFC-A Tag device that supports NFC's TNEP.

1. |connect_terminal|
#. Reset the kit.
#. Put the NFC Tag device anntena in the range of the NFC polling device.
   The NFC polling device selects the first service and exchanges basic data with it.
   After that, the service is deselected.
#. Observe the output in the terminal.

Dependencies
************

This sample uses the following |NCS| drivers:

* :ref:`st25r3911b_nfc_readme`

This sample uses the following |NCS| libraries:

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
