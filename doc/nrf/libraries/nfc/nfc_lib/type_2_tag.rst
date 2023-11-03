.. _type_2_tag:

Type 2 Tag
##########

.. contents::
   :local:
   :depth: 2

The Type 2 Tag implementation is based on the NFC Forum document *Type 2 Tag Technical Specification Version 1.0*.

A Type 2 Tag can be read and re-written, and the memory of the tag can be write protected.
The Type 2 Tag library implements a Type 2 Tag in read-only state.
Read only means that the memory of the tag is write protected and cannot be erased (formatted) or re-written by the polling device.

If you use the supplied library, you do not need to know about the actual :ref:`t2t_memory_layout` that is used to implement the tag.
You can just follow the steps outlined in :ref:`t2t_programming` to configure the tag data.

.. _t2t_memory_layout:

Memory layout
*************

This information is not required to program the NFC tag; you can just follow the steps outlined in :ref:`t2t_programming`.

The NFCT data is stored in RAM, in the same way as data for other nRF52 peripherals.
The data format that is implemented by this library is compliant to the Dynamic Memory Structure defined in the NFC Forum document *Type 2 Tag Technical Specification Version 1.0 2017-08-28 [T2T]*.

The Type 2 Tag memory has a size of 1024 bytes.
It is organized in the following way:

.. list-table::
   :header-rows: 1

   * - Block Number
     - Byte0
     - Byte1
     - Byte2
     - Byte3
     - Type
   * - 0
     - Internal 0
     - Internal 1
     - Internal 2
     - Internal 3
     - Reserved
   * - 1
     - Internal 4
     - Internal 5
     - Internal 6
     - Internal 7
     - Reserved
   * - 2
     - Internal 8
     - Internal 9
     - Lock 0
     - Lock 1
     - Reserved
   * - 3
     - CC 0
     - CC 1
     - CC 2
     - CC 3
     - Reserved
   * - 4
     - Data 0
     - Data 1
     - Data 2
     - Data 3
     - Data
   * - 5
     - Data 4
     - Data 5
     - ...
     - ...
     - Data
   * - ...
     - ...
     - ...
     - ...
     - ...
     - Data
   * - 15
     - ...
     - ...
     - Data 46
     - Data 47
     - Data
   * - 16
     - Data 48
     - Data 49
     - ...
     - ...
     - Data
   * - 17
     - ...
     - ...
     - ...
     - ...
     - Data
   * - ...
     - ...
     - ...
     - ...
     - ...
     - Data
   * - 251
     - ...
     - ...
     - ...
     - Data 991
     - Data
   * - 252
     - Lock
     - Lock
     - Lock
     - Lock
     - Lock
   * - 253
     - Lock
     - Lock
     - Lock
     - Lock
     - Lock
   * - 254
     - ...
     - ...
     - ...
     - ...
     - Lock
   * - 255
     - Lock
     - Lock
     - Lock
     - Rsvd
     - Lock

There are three block types:

* `Static reserved`_ (16 bytes)
* `Data`_ (992 bytes)
* `Dynamic Lock and Reserved bytes`_ (16 bytes)

Static reserved
===============

The static reserved bytes contain 10 internal bytes, 2 lock bytes, and 4 Capability Container bytes.

Internal bytes
--------------

The internal bytes depend on the UID length.
They are set according to the following table:

.. list-table::
   :header-rows: 1

   * - Byte number
     - 4-byte UID
     - 7-byte UID
     - 10-byte UID
   * - Internal0
     - UID0
     - UID0
     - UID0
   * - Internal1
     - UID1
     - UID1
     - UID1
   * - Internal2
     - UID2
     - UID2
     - UID2
   * - Internal3
     - UID3
     - BCC0 = CT ^ UID0 ^ UID1 ^ UID2
     - UID3
   * - Internal4
     - BCC0 = UID0 ^ UID1 ^ UID2 ^ UID3
     - UID3
     - UID4
   * - Internal5
     - 0xFF
     - UID4
     - UID5
   * - Internal6
     - 0xFF
     - UID5
     - UID6
   * - Internal7
     - 0xFF
     - UID6
     - UID7
   * - Internal8
     - 0xFF
     - BCC1 = UID3 ^ UID4 ^ UID5 ^ UID6
     - UID8
   * - Internal9
     - NFC Lib version
     - NFC Lib version
     - UID9

UID0 contains the manufacturer ID for Nordic Semiconductor and equals 0x5F.

CT stands for Cascade Tag byte and equals 0x88.

The UID bytes are stored in the nRF52 FICR registers.

If you want to use UID bytes other than the ones from the FICR registers, use the :c:func:`nfc_t2t_parameter_set` function with the :c:enumerator:`NFC_T2T_PARAM_NFCID1` parameter.
When choosing a custom UID, remember to follow the NFC Forum requirements.

Static Lock bytes
-----------------

Both Static Lock bytes (Lock 0 and Lock 1) are set to 0xFF, which means that the CC area and the first 48 bytes of the data area of the tag can only be read, not written.

CC bytes
--------

The Capability Container bytes (CC 0 - CC 3) are encoded as described in Section 4.6 of the Type 2 Tag Technical Specification.
They cannot be used for storing application data.
See the following table for the values of the CC bytes:

.. list-table::
   :header-rows: 1

   * - CC Byte Number
     - Meaning
     - Value
   * - CC0
     - NFC magic number
     - 0xE1
   * - CC1
     - Document version number (v1.0)
     - 0x10
   * - CC2
     - Size of data area (992 bytes / 8)
     - 0x7C
   * - CC3
     - R/W access (read only)
     - 0x0F

Data
====

Application data is organized in TLV blocks.
The data area can contain one or more TVL blocks, up to a maximum data size of 992 bytes.

A TLV block contains the following fields:

* **T** (required): block type
* **L** (optional, depending on the T value): block length in number of bytes (field length if present: 1 or 3 bytes)
* **V** (optional, depending on the T and L values): block value (present only if the L field is present and greater than 0), multibyte field with length equal to L field value

The following block types are defined:

.. list-table::
   :header-rows: 1

   * - TLV block name
     - Value of T field
     - Description
   * - NULL TLV
     - 0x00
     - Can be used for padding of memory areas.
       The NFC Forum Device shall ignore this.
   * - Lock Control TLV
     - 0x01
     - Defines details of the lock bits.
   * - Memory Control TLV
     - 0x02
     - Identifies reserved memory areas.
   * - NDEF Message TLV
     - 0x03
     - Contains an NDEF message.
   * - Proprietary TLV
     - 0xFD
     - Contains tag proprietary information.
   * - Terminator TLV
     - 0xFE
     - Contains the last TLV block in the data area.

To write data to the tag, use the Type 2 Tag library functions :c:func:`nfc_t2t_payload_set` or :c:func:`nfc_t2t_payload_raw_set`.
:c:func:`nfc_t2t_payload_set` configures a single NDEF TLV block based on a user-provided NDEF message.
:c:func:`nfc_t2t_payload_raw_set` does not configure a TLV block, but the provided data must be organized in a TLV structure.

Dynamic Lock and Reserved bytes
===============================

15 Dynamic Lock bytes are located after the data area. Each bit defines access conditions of 8 consecutive bytes of the tag data area, except for the first 48 bytes, whose access conditions are defined by `Static Lock bytes`_.
The Dynamic Lock bits are set to 1 to indicate that the tag is read-only.

The lock bytes are followed by 1 reserved byte (Rsvd) to get a multiple of 8 bytes.

.. _t2t_command_set:

Command set
***********

The current version of the Type 2 Tag library supports only one Type 2 Tag command type: the READ command.
When a READ command is received, the tag responds with the data that is stored for the tag.

READ command format
===================

The READ command has the following format:

.. list-table::
   :header-rows: 1

   * - Byte number
     - Description
     - Value
   * - 1
     - Code
     - 0x30
   * - 2
     - Number of the 4-byte data block to read from the tag data
     - 0x00 - 0xFF

The library can send either a READ response (success) or a NACK response (failure):

* A READ response contains the content of 4 data blocks starting with the requested data block (16 bytes).
* A NACK response contains the error code 0x0, 0x1, 0x4, or 0x5 (4 bits).

.. _t2t_programming:

Programming a tag
*****************

To program a tag, complete the following steps:

1. Implement a callback function that handles events from the Type 2 Tag library and register it:

   .. code-block:: c

      int err;
      /* Callback for NFC events */
      static void nfc_callback(void * context,
                               enum nfc_t4t_event event,
                               const uint8_t * data,
                               size_t data_length,
                               uint32_t flags)
      {
      ...
      }
      /* Set up NFC and register application callback for NFC events. */
      err = nfc_t2t_setup(nfc_callback, NULL);
      if (err) {
		printk("Cannot setup NFC T2T library!\n");
		return err;
      }

#. Configure the data for the tag.
   You can provide the data as NDEF message (recommended, see :ref:`nrf:ug_nfc_ndef`) or as a raw TLV structure (advanced usage, see Type 2 Tag :ref:`t2t_memory_layout`).

   * Set an NDEF message:

     .. code-block:: c

        uint8_t ndef_msg_buf[] = ...; // Buffer with the user NDEF message
        uint32_t len           = sizeof(ndef_msg_buf);
        /* Set created message as the NFC payload. */
        err = nfc_t2t_payload_set(ndef_msg_buf, len);
        if (err) {
        	printk("Cannot set payload!\n");
        	return err;
        }


   * Alternatively, set a TLV structure:

     .. code-block:: c

        uint8_t tlv_buf[] = ...; // Buffer with the user TLV structure
        uint32_t len           = sizeof(tlv_buf);
        /* Set created TLV structure as the NFC payload. */
        err = nfc_t2t_payload_raw_set(tlv_buf, len);
        if (err) {
        	printk("Cannot set raw payload!\n");
        	return err;
        }

#. Activate the NFC tag so that it starts sensing and reacts when an NFC field is detected:

   .. code-block:: c

      /* Start sensing NFC field. */
      err = nfc_t2t_emulation_start();
      if (err) {
           printk("Cannot start emulation!\n");
           return err;
      }

.. _nfc_api_type2:

API documentation
*****************

.. doxygengroup:: nfc_t2t_lib
   :project: nrfxlib
   :members:
