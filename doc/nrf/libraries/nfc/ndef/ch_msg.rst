.. _nfc_ch:

Connection Handover messages and records
########################################

.. contents::
   :local:
   :depth: 2

Connection Handover records and the corresponding messages are used to negotiate and activate an alternative communication carrier.
The negotiated communication carrier can then be used to perform certain activities between the two devices, such as Bluetooth® pairing.

Connection Handover records
***************************

The Connection Handover library provides functions for creating and encoding the following Connection Handover records:

* Handover Request record
* Handover Select record
* Handover Mediation record
* Handover Initiate record
* Handover Carrier record
* Local records:

  * Alternative Carrier record
  * Collision Resolution record


Connection Handover messages
****************************

The Connection Handover message library provides functions for encoding the following messages:

* Bluetooth LE OOB message
* Handover Select message
* Handover Request message
* Handover Mediation message
* Handover Initiate message

This library is used in the :ref:`peripheral_nfc_pairing` sample.

The following code sample demonstrates how to create a Handover Select message with one Alternative Carrier record that has a reference to the :ref:`Bluetooth LE OOB record <nfc_ndef_le_oob>`:

.. literalinclude:: ../../../../../samples/bluetooth/peripheral_nfc_pairing/src/main.c
    :language: c
    :start-after: include_startingpoint_pair_msg_rst
    :end-before: include_endpoint_pair_msg_rst

API documentation
*****************

.. _nfc_ndef_ch:

Connection Handover records
===========================

| Header file: :file:`include/nfc/ndef/ch.h`
| Source file: :file:`subsys/nfc/ndef/ch.c`

.. doxygengroup:: nfc_ndef_ch
   :project: nrf
   :members:


.. _nfc_pair_msg:

Connection Handover messages
============================

| Header file: :file:`include/nfc/ndef/ch_msg.h`
| Source file: :file:`subsys/nfc/ndef/ch_msg.c`

.. doxygengroup:: nfc_ndef_ch_msg
   :project: nrf
   :members:
