.. _nfc_tnep_poller:

NFC: TNEP Poller
################

The NFC TNEP Poller sample demonstrates how to use the :ref:`tnep_poller_readme` library to exchange data using an NFC Poller Device.

Overview
********

The sample can interact with the NFC Type 4 Tag.

Initially, the sample reads the NFC Type 4 Tag and looks for the TNEP Initial message.
After founding it, the first service from the message is selected and the Poller attempts to exchage data.
Next, the service is deselected.

Requirements
************
One of the following boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

* NFC Reader ST25R3911B Nucleo expansion board (X-NUCLEO-NFC05A1)
* NFC Type 4 Tag TNEP Device

Building and running
********************
.. |sample path| replace:: :file:`samples/nfc/tnep_poller`

.. include:: /includes/build_and_run.txt

Testing
=======
After programming the sample to your board, you can test it with an NFC-A Tag Device that supports NFC's TNEP.

1. |connect_terminal|
#. Reset the board.
#. Put the NFC Tag Device anntena in NFC Poller range.
   The NFC Poller Device selects the first service and exchanges basic data with it.
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

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* ``include/zephyr.h``
