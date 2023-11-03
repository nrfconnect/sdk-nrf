.. _type_4_tag:

Type 4 Tag
##########

.. contents::
   :local:
   :depth: 2

The Type 4 Tag implementation is based on the NFC Forum document *Type 4 Tag Technical Specification Version 1.0 2017-08-28 [T4T]*.

A Type 4 Tag must contain at least the NDEF tag application.
This application provides a file system that consists of at least two Elementary Files (EFs):

Capability container (CC)
  The CC is a read-only metafile that contains the version of the implemented specification, communication parameters of the tag, and properties of all the other EF files that are present on a Type 4 Tag platform.

NDEF file
  The NDEF file (see :ref:`t4t_format`) contains the NDEF message, which can be read or re-written depending on its file properties, which are defined in the CC file.

The Type 4 Tag library uses application protocol data units (APDUs) to communicate over the ISO-DEP (ISO14443-4A) protocol with any polling NFC device.
As with Type 2 Tag, Type 4 Tag also supports NFC-A listen mode.
In this mode, the tag can only wait for polling devices, but it does not actively start a connection.

The Type 4 Tag library supports three different modes of emulation:

.. list-table::
   :header-rows: 1

   * - Mode type
     - Description
   * - Raw ISO-DEP exchanges
     - All APDUs are signaled through the callback.
   * - Read-only T4T NDEF tag
     - The NDEF file cannot be modified.
       Only NFC field status and :c:enumerator:`NFC_T4T_EVENT_NDEF_READ` can be signaled through the callback.
   * - Read-write T4T NDEF tag
     - The NDEF file can be modified.
       The changes to the NDEF content are signaled through the callback.

.. _t4t_format:

NDEF file format
****************

The NDEF file is a file type that can be present in a Type 4 Tag platform.
It consists of following fields:

.. list-table::
   :header-rows: 1

   * - Field
     - Length
     - Description
   * - NLEN
     - 2 bytes
     - Length of the NDEF message in big-endian format.
   * - NDEF Message
     - NLEN bytes
     - NDEF message.
       See :ref:`nrf:ug_nfc_ndef`.

As you see, the NDEF file adds one additional field in comparison to the raw NDEF message.
It is called NLEN and is required for an NDEF file.
This field encodes the total length of the NDEF message.


.. _nfc_api_type4:

API documentation
*****************

.. doxygengroup:: nfc_t4t_lib
   :project: nrfxlib
   :members:
