.. _nfc_ndef:

Messages and records
####################

.. contents::
   :local:
   :depth: 2

Use the :ref:`nfc_ndef_msg` and :ref:`nfc_ndef_record` modules to create standardized messages and records that are not covered by other modules, or to create your own custom messages.

To generate an NDEF message, start by creating the records that make up the message.
Next, generate an empty message of the required length and add the records to the message.
Finally, encode and create the message.

By default, NDEF messages are encoded for the Type 2 Tag platform.
See :ref:`ug_nfc_ndef_format` for a description of the format.
You can choose to encode an NDEF file containing the NDEF message for the Type 4 Tag platform using the :ref:`nfc_t4t_ndef_file_readme` library.
In this case, an additional field is added in front of an NDEF message.

You can also encapsulate a message as payload for a record.
For example, a Handover Request message contains several Alternative Carrier records.
These records themselves are full messages that contain information about alternative carriers that can be used for further communication.

.. _nfc_ndef_record_gen:

Creating a record descriptor
****************************

You can provide the payload of the record in binary format or in a different format that is then converted into binary format.
The content of the record is defined in a record descriptor.

Use the :c:macro:`NFC_NDEF_RECORD_BIN_DATA_DEF` macro for binary data or the :c:macro:`NFC_NDEF_GENERIC_RECORD_DESC_DEF` macro for other data to create the record descriptor.
You must provide header information for the record (for example, ID and type) and the payload data to the macro.
For generic data that is not in binary format, you must also provide a payload constructor.
This constructor converts the payload data to the needed format.
For binary data, the module provides this constructor.

The following code example shows how to generate a record descriptor:

.. code-block:: c

   int err;
   uint32_t length;

   /// Declare record descriptor by macro - create and initialize an instance of
   ///   nfc_ndef_record_desc_t.
   NFC_NDEF_GENERIC_RECORD_DESC_DEF( record,
                                     3,
                                     id_string,
                                     sizeof(id_string),
                                     type_string,
                                     sizeof(type_string),
                                     payload_constructor, // implementation provided by user
                                     &payload_constructor_data // structure depends on user implementation
                                   );

   // If required, get the record size to length variable.
   err = nfc_ndef_record_encode( &NFC_NDEF_GENERIC_RECORD_DESC_DEF(record),
                                        NDEF_MIDDLE_RECORD,
                                        NULL,
                                        &length);



.. _nfc_ndef_msg_gen:

Creating a message
******************

A message consists of one or more records.
Accordingly, a message descriptor (that holds the content of the message) contains an array of pointers to record descriptors.

Use the :c:macro:`NFC_NDEF_MSG_DEF` macro to create the message descriptor and the array of pointers.
When creating the message descriptor, you specify how many records the message will contain, but you do not specify the actual records yet.

To add the records, use the :c:func:`nfc_ndef_msg_record_add` function.

After adding all records, call :c:func:`nfc_ndef_msg_encode` to actually create the message from the message descriptor.
:c:func:`nfc_ndef_msg_encode` internally calls :c:func:`nfc_ndef_record_encode` to encode each record.
The NDEF records are always encoded in long format.
If no ID field is specified, a record without ID field is generated.

The following code example shows how to create two messages:


.. code-block:: c

   int err;
   uint8_t buffer_for_message[512];
   uint8_t buffer_for_message_2[128];
   uint32_t length;

   // Declare message descriptor by macro - create and initialize an instance of
   //   nfc_ndef_msg_desc_t and an array of pointers to nfc_ndef_record_desc_t.
   // The declared message can contain up to 2 records.
   NFC_NDEF_MSG_DEF(my_message, 2);

   // Add record_1 and record_2 to the message.
   // record_1 and record_2 are record descriptors as created in the previous
   //   code example.
   err = nfc_ndef_msg_record_add( &NFC_NDEF_MSG(my_message), record_1);
   err = nfc_ndef_msg_record_add( &NFC_NDEF_MSG(my_message), record_2);

   // Get the message size to the length variable.
   err_t = nfc_ndef_msg_encode( &NFC_NDEF_MSG(my_message),
                                       NULL,
                                       &length);

   // Encode the message to buffer_for_message.
   ASSERT(length <= 512); // make sure the message fits into the buffer
   err_t = nfc_ndef_msg_encode( &NFC_NDEF_MSG(my_message),
                                       buffer_for_message,
                                       &length);

   // Clear the message description.
   nfc_ndef_msg_clear( &NFC_NDEF_MSG(my_message));

   // Add record_3 to the message.
   // record_3 is a record descriptors as created in the previous code example.
   err = nfc_ndef_msg_record_add( &NFC_NDEF_MSG(my_message), record_3);

   // Encode another message to buffer_for_message_2.
   length = 128; // amount of memory available for message
   err_t = nfc_ndef_msg_encode( &NFC_NDEF_MSG(my_message),
                                       buffer_for_message_2,
                                       &length);


.. _nfc_ndef_msg_rec:

Encapsulating a message
***********************

To encapsulate a message in a record so that it can be added to another message, use the :c:macro:`NFC_NDEF_NESTED_NDEF_MSG_RECORD_DEF` macro to create the record descriptor.
This record descriptor uses :c:func:`nfc_ndef_msg_encode` as payload constructor.
You can then add this record descriptor to a message like any other record descriptor.

The following code example shows how to encapsulate a message as payload for a record:


.. code-block:: c

   // nested_message_desc is a message descriptor

   // declare a record descriptor with an NDEF message nested in payload
   // create and initialize instance of nfc_ndef_record_desc_t
   NFC_NDEF_NESTED_NDEF_MSG_RECORD_DEF( compound_record,
                                        3,
                                        sizeof(id_string),
                                        id_string,
                                        type_string,
                                        sizeof(type_string),
                                        &nested_message_desc );

   // add compound record to a message like any other record
   err = nfc_ndef_msg_record_add( &NFC_NDEF_MSG(my_message), &NFC_NDEF_NESTED_NDEF_MSG_RECORD(compound_record));



API documentation
*****************

.. _nfc_ndef_msg:

NDEF messages
=============

| Header file: :file:`include/nfc/ndef/msg.h`
| Source file: :file:`subsys/nfc/ndef/msg.c`

.. doxygengroup:: nfc_ndef_msg
   :project: nrf
   :members:

.. _nfc_ndef_record:

NDEF records
============

| Header file: :file:`include/nfc/ndef/record.h`
| Source file: :file:`subsys/nfc/ndef/record.c`

.. doxygengroup:: nfc_ndef_record
   :project: nrf
   :members:
