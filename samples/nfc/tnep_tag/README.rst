.. _nfc_tnep_tag:

NFC: TNEP tag
#############

The TNEP tag sample demonstrates how to use the :ref:`tnep_tag_readme` library to exchange data using NFC's TNEP Protocol on an NFC Tag device.

Overview
********

The sample uses the Type 4 Tag as the tag transport layer.
Initially, the sample creates the initial NDEF message that contains the Services Parameter records and the NDEF records.

If the NFC polling device does not support the TNEP library, it can interact with the NDEF records.
If the poller supports the TNEP library, it can select the TNEP Service and exchange data with it.

Every TNEP Tag service has a callback structure that provides information to the application about Service State changes.
The sample has two TNEP services defined, each of them containing the NDEF text records.

Requirements
************

One of the following boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

User interface
**************

LED 1:
   On when the TNEP Tag is initialized.

LED 3:
   On when the TNEP service one is selected.

LED 4:
   On when the TNEP service two is selected.

Button 1:
   Press to provide the application data when the application service two is selected.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/tnep_tag`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, you can test it with an NFC-A polling device that supports NFC's Tag NDEF Exchange Protocol.

1. |connect_terminal|
#. Reset the board.
#. Touch the board antenna with the NFC polling device.
#. Observe the output in the terminal.
#. If the NFC polling device selects the service two, you have 27 seconds to press Button 1 to provide application data.
   If you do not do this, the NFC polling device will deselect the service.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`tnep_tag_readme`
* :ref:`nfc_ndef`
* :ref:`nfc_text`

In addition, it uses the following Zephyr libraries:

* ``include/sys/until.h``
