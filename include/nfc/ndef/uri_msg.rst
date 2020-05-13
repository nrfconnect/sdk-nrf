.. _nfc_uri:

URI messages and records
########################

URI messages contain exactly one URI record, which in turn contains an address that an NFC polling device can open.
In the most typical use case, the URI record contains a web address like "http\://www.nordicsemi.com" that the polling device opens in a web browser.

URI records consist of a URI field (the actual address) and a URI identifier that specifies the protocol of the URI and is prepended to the URI.
See :cpp:enum:`nfc_ndef_uri_rec_id` for the available protocols.

The :ref:`nfc_uri_record` module provides functions for creating the record, and the :ref:`nfc_uri_msg` module provides functions for creating and encoding the message.

The following code snippets show how to generate a URI message.
First, define the URI string and create a buffer for the message:

.. code-block:: c

   static const u8_t m_url[] =
       {'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.', 'c', 'o', 'm'}; //URL "nordicsemi.com"

   u8_t m_ndef_msg_buf[256];

Then create the URI message with one URI record.
As parameters, provide the URI identifier code (:cpp:member:`NFC_URI_HTTP_WWW` in this example), the URI string, the length of the URI string, the message buffer, and the size of the available memory in the buffer:


.. code-block:: c

   int err;

   err = nfc_ndef_uri_msg_encode( NFC_URI_HTTP_WWW,
                                   m_url,
                                   sizeof(m_url),
                                   m_ndef_msg_buf,
                                   &len);

   if (err < 0) {
        printk("Cannot encode message!\n");
	return err;
   }


API documentation
*****************

.. _nfc_uri_msg:

URI messages
============

| Header file: :file:`include/nfc/ndef/uri_msg.h`
| Source file: :file:`subsys/nfc/ndef/uri_msg.c`

.. doxygengroup:: nfc_uri_msg
   :project: nrf
   :members:

.. _nfc_uri_record:

URI records
===========

| Header file: :file:`include/nfc/ndef/uri_rec.h`
| Source file: :file:`subsys/nfc/ndef/uri_rec.c`

.. doxygengroup:: nfc_uri_rec
   :project: nrf
   :members:
