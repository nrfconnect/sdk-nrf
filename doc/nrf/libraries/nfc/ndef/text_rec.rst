.. _nfc_text:

Text records
############

.. contents::
   :local:
   :depth: 2

Text records contain descriptive text and can be added to any type of message.
This kind of record can be used in a message together with other records to provide extra information.
The Text Records module provides functions for creating and encoding text records.

Text records contain a payload (the actual text string) and a language identifier.
Note that a message can contain only one text record per language code, so you cannot add two different text records with language code "en" to the same message, for example.

The following code example shows how to generate a raw text message that contains a text record.

First, generate the text record.
In this example, the record contains the text "Hello World!" and the language code "en":

.. code-block:: c

   static const uint8_t en_payload[] = {
   	'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'
   };
   static const uint8_t en_code[] = {'e', 'n'};

   int err;

   /* Create NFC NDEF text record description in English */
   NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_en_text_rec,
			         UTF_8,
			         en_code,
			         sizeof(en_code),
			         en_payload,
			         sizeof(en_payload));

Next, create an empty message descriptor.
At this time, you specify how many records the message will contain, but you do not specify the actual records yet:

.. code-block:: c

   NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);

Add the record that you created to the message:

.. code-block:: c

   err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
   			      &NFC_NDEF_TEXT_RECORD_DESC(nfc_en_text_rec));
   if (err < 0) {
	   printk("Cannot add first record!\n");
	   return err;
   }


Finally, encode the message:

.. code-block:: c

   err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg),
				         buffer,
				         len);
   if (err < 0) {
	printk("Cannot encode message!\n");
        return err;
   }



API documentation
*****************

.. _nfc_text_record:

| Header file: :file:`include/nfc/ndef/text_rec.h`
| Source file: :file:`subsys/nfc/ndef/text_rec.c`

.. doxygengroup:: nfc_text_rec
   :project: nrf
   :members:
