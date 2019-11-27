.. _tnep_tag_readme:

Tag NDEF Exchange Protocol for NFC Tag Device
#############################################

.. include:: poller.rst
   :start-after: tnep_intro_start
   :end-before: tnep_intro_end


Initialization
==============
Before using the TNEP library in a Tag Device application, configure the NDEF message buffers and define the TNEP Services.

Configuring the NDEF message buffers
------------------------------------
The TNEP library communicates with the Tag Device using the NDEF message buffers.
The buffers are initialized all at the same time by :cpp:func:`nfc_tnep_tag_tx_msg_buffer_register`.

To use the NDEF message buffers, you must configure the following elements:
* Provide a pointer to two TX buffers: one TX buffer for emulating the NDEF message and one for encoding the new NDEF message.
* Set a maximum buffer size in bytes, which is valid for both buffers.

The buffers must be linked with the Tag Device layer in the application.

When the NFC Poller device writes a new NDEF Message inside the tag buffer, the application must inform the TNEP library by calling :cpp:func:`nfc_tnep_tag_rx_msg_indicate`.

Defining TNEP Services
----------------------
The TNEP Services are capabilities and features provided to the adjacent upper layer.
They must be defined for the TNEP library in order to create the Initial NDEF message.
At least one TNEP Service must be defined for the message to be created.

To create a TNEP Service, use the :c:macro:`NFC_TNEP_TAG_SERVICE_DEF` macro.

After defining the Services, create a table that lists all of them.
You can use :c:macro:`NFC_TNEP_TAG_SERVICE` to get the names of the Services created.

The Service table is given to the :cpp:func:`nfc_tnep_tag_initial_msg_create` function and the Service Parameter record of each Service is inserted into the Initial TNEP message.
A Service Parameter record contains parameters for communicating with the Service.

See the following code for an example of Service definition and TNEP initialization:

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
=============
After initialization, the TNEP library is in the Service Ready state and the NDEF message buffer contains Service Parameter records provided with the Initial TNEP message.

The Initial TNEP message can also include optional NDEF records that can interact with the NFC Poller device that does not support the protocol.

Make sure to call the :c:func:`nfc_tnep_tag_process` function in infinite loop in the application.

Receiving new messages
----------------------
When a new NDEF message appears in the buffer and it contains a Service Select record, the application can select this Service.
To do this, the application should indicate the service and inform the TNEP library by calling :cpp:func:`nfc_tnep_tag_rx_msg_indicate`.
The TNEP library will change its state to Service Selected.
At that point moving forward, the Service decryption message will not longer be provided.

After the successful Service selection, the select callback function of the Service will be called.
If the TNEP library was already in the Service Selected state at the moment of receiving the NDEF message with the Service Select record, the deselect callback of the previous service will be called before the select callback of the new service.

When TNEP is in the Service Selected state, the Service's new message callback will be called after successfully processing the new message.
Application data can be added to the reply message with :cpp:func:`nfc_tnep_tag_tx_msg_app_data`.
This function can be called from the Service Selected callback or from any other context, for example a different thread.

If the Tag Device application has no more data, it will reply by using :cpp:func:`nfc_tnep_tag_tx_msg_no_app_data`.
If the application does not reply before the expiration on the time period specified by the Service's initialization parameters, the Service will be deselected by the Reader/Writer device.

The following code demonstrates how to exchange the NDEF messages using the TNEP library after initialization:

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
