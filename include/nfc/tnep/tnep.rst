.. _tnep_tag:

Tag NDEF Exchange Protocol for NFC Tag Device
#############################################

TNEP (The Tag NDEF Exchange Protocol) is an application-level protocol for sending or
retrieving application data units, in the form of NFC Data Exchange Format (NDEF) messages,
between one Reader/Writer and an NFC Tag Device. It operates between Tag X Tag and NDEF application layer.

Initialization
==============
Before using the TNEP library in a tag device application it is required to set the NDEF message buffer and define TNEP services.

NDEF Message
------------
The TNEP library communicates with Tag X Tag using NDEF message buffer.
The buffer is initialized using :cpp:func:`nfc_tnep_tx_msg_buffer_register`.
It is required to provide a pointer to the buffer and a maximum buffer size in bytes.
Linking the buffer with Tag X Tag layer should be done by a user application.

When a new message appear inside the buffer the application should inform the TNEP library
calling :cpp:func:`nfc_tnep_rx_msg_indicate`.

Services
--------
The word services mean the capabilities and features provided to the adjacent upper layer.
The definition macro :c:macro:`NFC_TNEP_SERVICE_DEF` can be used to easily create a TNEP service.
After defining services it is required to create a table with all of them. It is recommended to use :c:macro:`NFC_TNEP_SERVICE`.
The services containing table is given to :cpp:func:`nfc_tnep_init` function.

Example service definition and TNEP initialization is shown in code below:

.. code-block:: c

	#define SERVICES_NUMBER 1
	#define NFC_TNEP_SERVICE_DEF(service_name, uri, uri_length, mode, t_wait, n_wait,
								 select_cb, deselect_cb message_cb, timeout_cb, error_cb)

	struct nfc_tnep_service services[SERVICES_NUMBER] = {
					NFC_TNEP_SERVICE(service_name),
			};

	nfc_tnep_tx_msg_buffer_register(buffer, buffer_length);

	nfc_tnep_init(services, SERVICES_NUMBER);

Workflow
========
After initialization the TNEP library is in the Service Ready state. The NDEF message buffer contains services description records.
It is also possible to add an application record to the NDEF message calling :cpp:func:`nfc_tnep_tx_msg_app_data`.

The :c:func:`nfc_tnep_process` should be called in infinite loop inside application.

New message
-----------
When a new message appear inside the buffer an application should inform the TNEP library
calling the :cpp:func:`nfc_tnep_rx_msg_indicate`.

If NDEF message contains service select record the TNEP library change it state to Service Selected.
The services decryption message will not longer be provided.
After successful service selection the service's select callback function will be called.
If TNEP library was already in Service Selected state, the previous service's deselect callback will be called before the new service's select callback.

When TNEP is in the Service Select state, after successfully processing new message, the service's new message callback will be called.
Application data can be added to reply message using :cpp:func:`nfc_tnep_tx_msg_app_data`.
If application won't reply until the time period specified by service's initialization parameters expires,
service will be deselected by Reader/Writer device.

.. code-block:: c

	static s8_t training_service_selected(void)
	{
		/* service selection function body */

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
		nfc_tnep_rx_msg_indicate();
	}

	main()
	{
		/*initialization code, application code*/
		while (1) {
			nfc_tnep_process();
		}
	}


Disabled TNEP
-------------
If TNEP library will no longer be used in application it can be disabled calling :cpp:func:`nfc_tnep_uninit`.

API documentation
*****************

| Header file: :file:`include/tnep/tag.h`
| Source file: :file:`subsys/tnep/tag.c`

.. doxygengroup:: nfc_tnep
   :project: nrf
   :members:
