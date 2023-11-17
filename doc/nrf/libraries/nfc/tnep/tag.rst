.. _tnep_tag_readme:

TNEP for tag device
###################

.. contents::
   :local:
   :depth: 2

This library provides an implementation of the Tag NDEF Exchange Protocol (TNEP) for an NFC tag device.

It is used in the :ref:`nfc_tnep_tag` sample.

Initialization
**************

Before using the TNEP library in an NFC tag application, you must configure the NDEF message buffers and define the TNEP services.

Configuring NDEF message buffers
================================

The tag library communicates with the :ref:`nrfxlib:type_4_tag` library using NDEF message buffers.
The buffers are initialized all at the same time by the :c:func:`nfc_tnep_tag_tx_msg_buffer_register` function.

To use the NDEF message buffers, you must configure the following elements:

* Provide a pointer to two TX buffers - one for the NDEF message used for tag emulation and another one for encoding the new NDEF message for later usage.
* Set a maximum buffer size in bytes, which is valid for both buffers.

The buffers must be linked with the tag layer in the application.

When the polling device writes a new NDEF message to the tag buffer, the application must inform the tag library by calling the :c:func:`nfc_tnep_tag_rx_msg_indicate` function.

Defining TNEP services
======================

TNEP provides mechanisms for creating services and exchanging data through these services.
The client (application or upper-layer protocol) must define these services and use them for data transfer.
At least one TNEP service must be defined for the initial NDEF message to be created.

To create a TNEP service, use the :c:macro:`NFC_TNEP_TAG_SERVICE_DEF` macro.
The service data is stored in the persistent storage.

Initial TNEP NDEF message
=========================

When calling the :c:func:`nfc_tnep_tag_initial_msg_create` function, the service data is loaded and the Service Parameter record of each service is inserted into the Initial TNEP message.
A Service Parameter record contains parameters for communicating with the service.
You can also encode additional NDEF records into the Initial TNEP message in the following way:

#. Pass a callback to the :c:func:`nfc_tnep_tag_initial_msg_create` function.
#. Define an additional record inside the callback and call the :c:func:`nfc_tnep_initial_msg_encode` function.

See the following code for an example of the Initial message encoding:

.. literalinclude:: ../../../../../samples/nfc/tnep_tag/src/main.c
    :language: c
    :start-after: include_startingpoint_initial_msg_cb_rst
    :end-before: include_endpoint_initial_cb_msg_rst

The following code shows how to create the Initial NDEF message:

.. literalinclude:: ../../../../../samples/nfc/tnep_tag/src/main.c
    :language: c
    :start-after: include_startingpoint_initial_msg_rst
    :end-before: include_endpoint_initial_msg_rst


See also the following code for an example of service definition and TNEP initialization:

.. code-block:: c

	#define NFC_TNEP_TAG_SERVICE_DEF(service_name, uri, uri_length, mode, t_wait, n_wait, mac_msg_size
								 select_cb, deselect_cb message_cb, timeout_cb, error_cb)

	nfc_tnep_tx_msg_buffer_register(buffer, swap_buffer, buffer_length);

	nfc_tnep_init(event, event_cnt, nfc_txt_rw_payload_set);

	nfc_tXt_setup();

	nfc_tnep_tag_initial_msg_create(1, NULL);

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
To do this, the application should inform the TNEP tag library of the received NDEF message by calling the :c:func:`nfc_tnep_tag_rx_msg_indicate` function.
The tag library will then change its state to Service Selected.
From that point, the service description message will no longer be provided.

After the successful service selection, the select callback function of the service is called.
If the tag library was already in Service Selected state when receiving the NDEF message with the Service Select record, the deselect callback of the previous service is called before the select callback of the new service.

When the library is in the Service Selected state, the service's new message callback is called after successfully processing the new message.
Application data can be added to the reply message using the :c:func:`nfc_tnep_tag_tx_msg_app_data` function.
It can be called from the Service Selected callback or from any other context, for example a different thread.

If the tag application has no more data, it will reply by using the :c:func:`nfc_tnep_tag_tx_msg_no_app_data` function.
If the application does not reply before the expiration on the time period specified by the service's initialization parameters, the service will be deselected by the polling device.

The following code demonstrates how to exchange NDEF messages using the tag library after initialization:

.. literalinclude:: ../../../../../samples/nfc/tnep_tag/src/main.c
    :language: c
    :start-after: include_startingpoint_tnep_service_rst
    :end-before: include_endpoint_tnep_service_rst

The following code demonstrates how to process the TNEP library:

.. code-block:: c

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
