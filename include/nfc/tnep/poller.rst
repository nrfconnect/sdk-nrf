.. _tnep_poller_readme:

Tag NDEF Exchange Protocol for NFC Poller Device
################################################

.. tnep_intro_start

The Tag NDEF Exchange Protocol (TNEP) is an application-level protocol for sending or retrieving application data units between a Poller Device and an NFC Tag Device.

The data units take the form of NFC Data Exchange Format (NDEF) messages.
The protocol operates between the NDEF application layer and the Tag Device, that is a device that supports at least one communication protocol for NDEF.
The Poller is an NFC device in the Poll mode, in which it sends commands and receives responses.

The NFC Tag Device supports all tag types from Type 2 Tag to Type 5 Tag.
The Poller communicates with the type of tag corresponding to the NFC Tag Device type.
The communication occurs through Poller's Reader/Writer role.

.. tnep_intro_end

Initialization
==============

To initialize the Poller library, call :cpp:func:`nfc_tnep_poller_init` and provide the TX buffer and callback structure as parameters.
To ensure that the Poller Device discovers the Tag Device, the initialization must be based on the Tag Device type.

Configuration
=============

After reading the NDEF message from the NFC Tag Device, the library can use the :cpp:func:`nfc_tnep_poller_svc_search` function to look for the Initial NDEF message that contains the Service Parameters records.

If the NDEF Initial message has valid Service Parameters records, the Poller Device can select a service using :cpp:func:`nfc_tnep_poller_svc_select`.

After finishing all operations on the service, the service should be deselected by using one of the following options:

* selecting another service
* using :cpp:func:`nfc_tnep_poller_svc_deselect`


Exchanging data with Single response communication mode
-------------------------------------------------------
The Poller Device can use the Single response communication mode to exchange the NDEF Message according to NFC Forum TNEP specification (chapter 5).

Exchanging data is possible only when a service is selected.
The data is exchanged in the NDEF Read Procedure or the NDEF Write Procedure.

To exchange data, use :cpp:func:`nfc_tnep_poller_svc_update` or :cpp:func:`nfc_tnep_poller_on_ndef_read`.

.. note::
    These operations are asynchronous.

When the NFC Poller device finishes the NDEF Read Procedure or the NDEF Write Procedure, the application should inform the library about this by calling :cpp:func:`nfc_tnep_poller_on_ndef_read` or :cpp:func:`nfc_tnep_poller_on_ndef_update`, respectively.

API documentation
=================

| Header file: :file:`include/tnep/poller.h`
| Source file: :file:`subsys/tnep/poller.c`

.. doxygengroup:: nfc_tnep_poller
   :project: nrf
   :members:
