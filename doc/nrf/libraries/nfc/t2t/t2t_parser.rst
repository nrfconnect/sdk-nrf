.. _nfc_t2t_parser_readme:

Type 2 Tag parser
#################

.. contents::
   :local:
   :depth: 2

The |NCS| provides functionality to read the content of an NFC tag.
Note that an nRF5 Series IC does not provide the hardware that is required for a polling device.
If you want to create a polling device, you can use an ST25R3911B NFC shield together with an nRF5 Development Kit.

Using the :ref:`st25r3911b_nfc_readme`, you can read raw data from a Type 2 Tag, such as:

* Number of TLV structures in the tag
* Type 2 Tag internal bytes
* Type 2 Tag lock bytes
* Type 2 Tag capability container
* Array of pointer to the TLV structures of the tag

See the :ref:`nfc_tag_reader` sample for more information on how to do this.

The following code example shows how to parse the raw data from a Type 2 Tag:

.. code-block:: c

        /* Buffer with raw data from a Type 2 Tag. */
        uint8_t raw_tag_data[] = ..;

        /* Declaration of Type 2 Tag structure which
         * can contain a maximum of 10 TLV blocks.
         */
	NFC_T2T_DESC_DEF(tag_data, 10);
	struct nfc_t2t *t2t = &NFC_T2T_DESC(tag_data);

	err = nfc_t2t_parse(t2t, raw_tag_data);
	if (err) {
		printk("Not enough memory to read the whole tag. Printing what has been read.\n");
	}

        /* Print the tag data. */
	nfc_t2t_printout(t2t);

Note that the data can be further parsed by the :ref:`nfc_ndef_parser_readme`, given that the type of the analyzed TLV block is NDEF TLV.

API documentation
*****************

NFC Type 2 Tag parser
---------------------

| Header file: :file:`include/nfc/t2t/t2t_parser.h`
| Source file: :file:`subsys/nfc/t2t/parser.c`

.. doxygengroup:: nfc_t2t_parser
   :project: nrf
   :members:

Type 2 Tag TLV blocks
---------------------

| Header file: :file:`include/nfc/t2t/tlv_block.h`

.. doxygengroup:: nfc_t2t_tlv_block
   :project: nrf
   :members:
