.. _nfc:

NFC NDEF libraries
##################

The NFC Data Exchange Format (NDEF) is a binary format that is used for data exchange through NFC.
This data format is commonly used in NFC devices (like smartphones) and NFC tags.

NDEF messages consist of one or more records of different types.
The type indicates the kind of data that the record contains, and the series of record types in a message defines the message type.
For example, a URI message contains one record that encodes a URL string.

`NDEF message and record format`_ explains in more detail how NDEF messages are built up.

The |NCS| provides modules that help you generate and encode NDEF messages and records.
There are modules for specific message and record types as well as a generic generator that you can use to easily implement other standardized records and messages or even create your own records.
See `Message types`_ for more information.
If you use the provided modules, you do not need deep knowlege of the NDEF specification to start using NFC.

The NFC NDEF libraries are used in the following samples:

* :ref:`record_text`
* :ref:`writable_ndef_msg`

.. _nfc_ndef_format_dox:

NDEF message and record format
******************************

NDEF data is structured in messages.
Each message consists of one or more records, which are made up of a header and the record payload.
The :ref:`nfc_ndef_format_header` contains metadata about, amongst others, the payload type and length.
The :ref:`nfc_ndef_format_payload` constitutes the actual content of the record.

.. figure:: /images/ndef_msg.svg
   :alt: Structure of an NDEF message and record

   Structure of an NDEF message and record


.. _nfc_ndef_format_header:

Record header
=============

The NDEF record header consists of the following fields:

.. list-table::
   :header-rows: 1

   * - Field
     - Length
     - Required
     - Description
   * - Flags and TNF
     - 1 byte
     - yes
     - See :ref:`nfc_ndef_format_flags`.
   * - Type Length
     - 1 byte
     - yes
     - Specifies the length of the payload type field.
       Required, but may be zero.
   * - Payload Length
     - 1 or 4 bytes
     - yes
     - Specifies the length of the payload.
       Either 1 byte or 4 byte long, depending on the SR flag. Required, but may be zero.
   * - ID Length
     - 1 byte
     - no
     - Required if the IL flag is set.
       Specifies the size of the Payload ID field.
   * - Payload Type
     - variable
     - no
     - Required if the Type Length is > 0.
       Specifies the type of the NDEF record payload.
   * - Payload ID
     - variable
     - no
     - Required if the IL flag is set and the ID Length is > 0.
       Specifies the ID of the NDEF record payload.

.. _nfc_ndef_format_flags:

Flags and TNF
-------------

.. figure:: /images/ndef_header_flags.svg
   :alt: Flags and TNF byte

   Flags and TNF byte


The Flags and TNF byte contains the following flags:

MB (Message Begin) and ME (Message End):
   Specify the position of the NDEF record within the message.
   The MB flag is set for the first record in the message.
   Similarly, the ME flag is set for the last record in the message.
   If a record is the only record in a message, both flags are set.
CF (Chunk Flag):
   Used for chunked payloads (a payload that is partitioned into multiple records).
   Set in all chunks of the record except for the last one.
   Note, however, that chunking is not supported by this library.
SR (Short Record):
   Used to determine the size of the payload length field.
   If the flag is set, the Payload Length occupies 1 byte; otherwise it is 4 bytes long.
   Note that the NDEF generator supports a Payload Length of 4 bytes only at the moment.
IL (ID Length present):
   Indicates whether an ID Length field is present in the header.
   If the flag is set, the ID Length field is present.
TNF (Type Name Format):
   Specifies the structure of the Payload Type field and how to interpret it.
   The following values are allowed (square brackets contain documentation reference related to the specific type):

   +-------+------------------------------------+
   | Value | Type Name Format                   |
   +=======+====================================+
   | 0x00  | Empty                              |
   +-------+------------------------------------+
   | 0x01  | NFC Forum well-known type [NFC RTD]|
   +-------+------------------------------------+
   | 0x02  | Media-type [RFC 2046]              |
   +-------+------------------------------------+
   | 0x03  | Absolute URI [RFC 3986]            |
   +-------+------------------------------------+
   | 0x04  | NFC Forum external type [NFC RTD]  |
   +-------+------------------------------------+
   | 0x05  | Unknown                            |
   +-------+------------------------------------+
   | 0x06  | Unchanged                          |
   +-------+------------------------------------+
   | 0x07  | Reserved                           |
   +-------+------------------------------------+

.. _nfc_ndef_format_payload:

Record payload
==============

The content of the payload is application-specific and related to the type of the record.
For example, in URI records, the payload contains a web address of the page that the polling device should open.

Note that the payload of an NDEF record can contain a nested NDEF message.
This nested message must be a full NDEF message, consisting of one or multiple NDEF records with the appropriate setting of MB and ME flags.


Message types
*************

The |NCS| provides modules to generate valid NDEF records and compose them into NDEF messages.

Currently, the following messages and records are supported:

.. toctree::
   :maxdepth: 1

   le_oob_rec
   nfc_text
   nfc_uri
   nfc_ndef
