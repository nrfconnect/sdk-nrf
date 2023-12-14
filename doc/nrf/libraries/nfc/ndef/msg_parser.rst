.. _nfc_ndef_parser_readme:

Parser for messages and records
###############################

.. contents::
   :local:
   :depth: 2

When using an NFC polling device, you can use the NDEF message parser module to interpret the NDEF messages that are read from the NFC tag.
See the documentation for the :ref:`nfc_t2t_parser_readme` for more information on how to read the content of the tag.

This library can also be used together with the :ref:`nrfxlib:nfc_api_type4` in writable mode to parse the written NDEF message.

The library provides parsers for NDEF messages and records.
The message parser module provides functionality for parsing NDEF messages and printing out their content.
It then uses the NDEF record parser module for parsing NDEF records to record descriptions.

When parsing a message, you must provide memory for the message descriptor instance which holds pointers to the record descriptors.
The record descriptors, in turn, point to the locations of the record fields which are located in the NFC data that was parsed.
These fields are not copied.

The following code example shows how to parse an NDEF message, after you have used the :ref:`nfc_t2t_parser_readme` to read the NFC data:

.. code-block:: c

   int  err;
   uint8_t desc_buf[NFC_NDEF_PARSER_REQUIRED_MEM(MAX_NDEF_RECORDS)];
   size_t desc_buf_len = sizeof(desc_buf);

   err = nfc_ndef_msg_parse(desc_buf,
                            &desc_buf_len,
			    ndef_msg_buff,
			    &nfc_data_len);

   if (err) {
        printk("Error during parsing an NDEF message, err: %d.\n", err);
   }

   nfc_ndef_msg_printout((struct nfc_ndef_msg_desc *) desc_buf);

The :ref:`nfc_tag_reader` sample shows how to use the library in an application.

API documentation
*****************

NDEF message parser API
-----------------------

| Header file: :file:`include/nfc/ndef/msg_parser.h`
| Source file: :file:`subsys/nfc/ndef/msg_parser.c`

.. doxygengroup:: nfc_ndef_msg_parser
   :project: nrf
   :members:

NDEF record parser API
----------------------

| Header file: :file:`include/nfc/ndef/record_parser.h`
| Source file: :file:`subsys/nfc/ndef/record_parser.c`

.. doxygengroup:: nfc_ndef_record_parser
   :project: nrf
   :members:
