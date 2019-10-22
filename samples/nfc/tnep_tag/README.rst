.. _nfc_tnep_tag:

NFC: TNEP Tag
#############

The NFC TNEP Tag sample demonstrates how to use the :ref:`tnep_tag_readme` library to exchange
data using NFC TNEP Protocol on the NFC Tag Device.

Overview
********

The sample shows how to use NFC TNEP Protocol on the NFC Tag Device. Sample used
NFC Type 4 Tag as a TAG transport layer. Initially, the sample creates The Initial NDEF message which contains the Services Parameter Records and NDEF Records. NFC Poller Device which does not support TNEP Protocol can interact with NDEF Records. When the NFC Supports TNEP Protocol it can select the TNEP Service and exchange data with it. Every TNEP Tag service has its callback structure which provides information to user application about service state changes. The sample has defined two TNEP services, each of them contains the NDEF Text Records inside.

Requirements
************
One of the following boards:

  * nRF52 Development Kit board (PCA10040)
  * nRF52840 Development Kit board (PCA10056)

User interface
**************

Button 1:
  Press this button to provides application data when application service two is selected.

Building and running
********************
.. |sample path| replace:: :file:`samples/nfc/tag_reader`

.. include:: /includes/build_and_run.txt

Testing
=======
After programming the sample to your board, you can test it with an NFC-A Poller Device
which support NFC TNEP Protocols.

1. |connect_terminal|
#. Reset the board.
#. Touch board antenna with the NFC Poller Device
#. Observe the output in the terminal.
#. If the NFC Poller Device select the service two you
   have 27 seconds to press the Button 1 to provide application data.
   If you do not do this, then the NFC Poller device will deselect the service.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`tnep_tag_readme`
* :ref:`nfc_ndef`
* :ref:`nfc_text`

In addition, it uses the following Zephyr libraries:

* ``include/misc/until.h``
