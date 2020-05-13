.. _tnep_tag_readme:

TNEP for tag device
###################

This library provides an implementation of the Tag NDEF Exchange Protocol (TNEP) for an NFC tag device.

It is used in the :ref:`nfc_tnep_tag` sample.

Initialization
**************

Before using the TNEP library in an NFC tag application, you must configure the NDEF message buffers and define the TNEP services.

Configuring NDEF message buffers
================================

The tag library communicates with the :ref:`nrfxlib:type_4_tag` library using NDEF message buffers.
The buffers are initialized all at the same time by :cpp:func:`nfc_tnep_tag_tx_msg_buffer_register`.

To use the NDEF message buffers, you must configure the following elements:

* Provide a pointer to two TX buffers - one for the NDEF message used for tag emulation and another one for encoding the new NDEF message for later usage.
* Set a maximum buffer size in bytes, which is valid for both buffers.

The buffers must be linked with the tag layer in the application.

When the polling device writes a new NDEF message to the tag buffer, the application must inform the tag library by calling :cpp:func:`nfc_tnep_tag_rx_msg_indicate`.

Defining TNEP services
======================

TNEP provides mechanisms for creating services and exchanging data through these services.
The client (application or upper-layer protocol) must define these services and use them for data transfer.
At least one TNEP service must be defined for the initial NDEF message to be created.

To create a TNEP service, use the :c:macro:`NFC_TNEP_TAG_SERVICE_DEF` macro.

After defining the services, create a table that lists all of them.
You can use :c:macro:`NFC_TNEP_TAG_SERVICE` to get the names of the created services.

The service table is given to the :cpp:func:`nfc_tnep_tag_initial_msg_create` function and the Service Parameter record of each service is inserted into the initial TNEP message.
A Service Parameter record contains parameters for communicating with the service.

See the following code for an example of service definition and TNEP initialization:

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

Configuration
*************
After initialization, the tag library is in the Service Ready state and the NDEF message buffer contains Service Parameter records provided with the initial TNEP message.

The initial TNEP message can also include NDEF records that are not related with TNEP.
The polling device, which does not support TNEP, can still interact with them.

Make sure to call the :c:func:`nfc_tnep_tag_process` function in an infinite loop in the application.

Receiving new messages
**********************

When a new NDEF message appears in the buffer and it contains a Service Select record, the application can select this service.
To do this, the application should inform the TNEP tag library of the received NDEF message by calling :cpp:func:`nfc_tnep_tag_rx_msg_indicate`.
The tag library will then change its state to Service Selected.
From that point, the service description message will not longer be provided.

After the successful service selection, the select callback function of the service is called.
If the tag library was already in Service Selected state when receiving the NDEF message with the Service Select record, the deselect callback of the previous service is called before the select callback of the new service.

When the library is in the Service Selected state, the service's new message callback is called after successfully processing the new message.
Application data can be added to the reply message with :cpp:func:`nfc_tnep_tag_tx_msg_app_data`.
This function can be called from the Service Selected callback or from any other context, for example a different thread.

If the tag application has no more data, it will reply by using :cpp:func:`nfc_tnep_tag_tx_msg_no_app_data`.
If the application does not reply before the expiration on the time period specified by the service's initialization parameters, the service will be deselected by the polling device.

The following code demonstrates how to exchange NDEF messages using the tag library after initialization:

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
