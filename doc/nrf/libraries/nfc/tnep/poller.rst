.. _tnep_poller_readme:

TNEP for polling device
#######################

.. contents::
   :local:
   :depth: 2

The poller library provides functionality to implement the Tag NDEF Exchange Protocol (TNEP) for a polling device.

The poller library is used in the :ref:`nfc_tnep_poller` sample.

Initialization
**************

To initialize the poller library, call :c:func:`nfc_tnep_poller_init` and provide the TX buffer and callback structure as parameters.
To ensure that the polling device discovers the tag, the initialization must be performed as required by the tag type.

Configuration
*************

After reading the NDEF message from the NFC tag, the library can use the :c:func:`nfc_tnep_poller_svc_search` function to look for the initial NDEF message that contains the Service Parameters records.

If the initial message has valid Service Parameters records, the polling device can select a service using :c:func:`nfc_tnep_poller_svc_select`.

After finishing all operations on the service, the service should be deselected by using one of the following options:

* Selecting another service
* Using :c:func:`nfc_tnep_poller_svc_deselect`


Exchanging data with single response communication mode
=======================================================

The polling device can use single response communication mode to exchange the NDEF message according to the NFC Forum TNEP specification (chapter 5).

Exchanging data is possible only when a service is selected.
The data is exchanged in the NDEF read procedure or the NDEF write procedure.

To exchange data, use :c:func:`nfc_tnep_poller_svc_write` or :c:func:`nfc_tnep_poller_on_ndef_read`.

.. note::
    These operations are asynchronous.

When the polling device finishes the NDEF read procedure or the NDEF write procedure, the application should inform the library about this by calling :c:func:`nfc_tnep_poller_on_ndef_read` or :c:func:`nfc_tnep_poller_on_ndef_write`, respectively.

API documentation
*****************

| Header file: :file:`include/tnep/poller.h`
| Source file: :file:`subsys/tnep/poller.c`

.. doxygengroup:: nfc_tnep_poller
   :project: nrf
   :members:
