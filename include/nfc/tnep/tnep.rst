.. _tnep_tag_readme:

Tag NDEF Exchange Protocol for NFC Tag Device
#############################################

TNEP (The Tag NDEF Exchange Protocol) is an application-level protocol for sending or
retrieving application data units, in the form of NFC Data Exchange Format (NDEF) messages,
between one Reader/Writer and an NFC Tag Device. It operates between Type X Tag and NDEF application layer.

Initialization
==============
Before using the TNEP library in a tag device application it is required to set the NDEF message buffers and define TNEP services.

NDEF Message
------------
The TNEP library communicates with Type X Tag using NDEF message buffers.
The buffers are initialized using :cpp:func:`nfc_tnep_tag_tx_msg_buffer_register`.
It is required to provide a pointer to the two buffers and a maximum buffer size in bytes.
Linking the buffers with Type X Tag layer should be done by a user application. This library
needs two TX buffer because when one buffer is used to emulate Tag NDEF Message, in second
the new NDEF Message is encoded.

When the NFC Polling Device write a new NDEF Message inside the Tag buffer the application should inform the TNEP library
calling :cpp:func:`nfc_tnep_tag_rx_msg_indicate`.

Services
--------
The word services mean the capabilities and features provided to the adjacent upper layer.
The definition macro :c:macro:`NFC_TNEP_TAG_SERVICE_DEF` can be used to easily create a TNEP service.
After defining services it is required to create a table with all of them. It is recommended to use :c:macro:`NFC_TNEP_TAG_SERVICE` to
get the names of created services.
The services containing table is given to :cpp:func:`nfc_tnep_tag_initial_msg_create` function to create the Initial TNEP message.

Example service definition and TNEP initialization is shown in code below:

.. code-block:: c

	#define NFC_TNEP_TAG_SERVICE_DEF(service_name, uri, uri_length, mode, t_wait, n_wait, mac_msg_size
								 select_cb, deselect_cb message_cb, timeout_cb, error_cb)

	NFC_NDEF_MSG_DEF(ndef_msg, 16);

	static struct nfc_tnep_tag_service services[] = {
		NFC_TNEP_TAG_SERVICE(service_name),
	};

	nfc_tnep_tx_msg_buffer_register(buffer, swap_buffer, buffer_length);

	nfc_tnep_init(event, event_cnt, &NFC_NDEF_MSG(ndef_msg),
		      nfc_txt_rw_payload_set);

	nfc_tXt_setup();

	nfc_tnep_tag_initial_msg_create(services, ARRAY_SIZE(services), NULL, 0);

	nfc_tXt_emulation_start();

Workflow
========
After initialization the TNEP library is in the Service Ready state. The NDEF message buffer contains services description records. The initial TNEP message can contains also optional NDEF Records which can interact with the NFC Polling Device which does not support TNEP Protocol.

The :c:func:`nfc_tnep_tag_process` should be called in infinite loop inside application.

New message
-----------
When a new message appear inside the buffer an application should inform the TNEP library
calling the :cpp:func:`nfc_tnep_tag_rx_msg_indicate`.

If NDEF message contains service select record the TNEP library change it state to Service Selected.
The services decryption message will not longer be provided.
After successful service selection the service's select callback function will be called.
If TNEP library was already in Service Selected state, the previous service's deselect callback will be called before the new service's select callback.

When TNEP is in the Service Select state, after successfully processing new message, the service's new message callback will be called.
Application data can be added to reply message using :cpp:func:`nfc_tnep_tag_tx_msg_app_data`. This function can be called form service selected callback or from the other context. If Tag application has no more data then it reply using :cpp:func:`nfc_tnep_tag_tx_msg_no_app_data`.
If application won't reply until the time period specified by service's initialization parameters expires,
service should be deselected by Reader/Writer device.

.. code-block:: c

	static void training_service_selected(void)
	{
		/* service selection function body */
                nfc_tnep_tag_tx_msg_app_data(app_records, records_cnt);

		return 0;
	}

	static void training_service_deselected(void)
	{
		/* service deselection function body */
	}

	static void training_service_new_message(void)
	{
		/* new application data function body */

		/* Add application data reply*/
		nfc_tnep_tx_msg_app_data(app_record);
	}

	tag_x_tag_handler()
	{
		nfc_tnep_tag_rx_msg_indicate();
	}

	main()
	{
		/*initialization code, application code*/
		while (1) {
			nfc_tnep_tag_process();
		}
	}

API documentation
*****************

| Header file: :file:`include/tnep/tag.h`
| Source file: :file:`subsys/tnep/tag.c`

.. doxygengroup:: nfc_tnep_tag
   :project: nrf
   :members:
