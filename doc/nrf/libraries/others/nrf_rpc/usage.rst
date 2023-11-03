.. _nrf_rpc_usage:

Implementing remote procedure calls
###################################

.. contents::
   :local:
   :depth: 2

A specific API can be used remotely if RPC encoders and RPC decoders are provided for it.
On one side, there are encoders that encode parameters and send commands or events.
On the other side, there are decoders that decode and execute a specific procedure.

The main goal of nRF RPC API is to allow the creation of RPC encoders and RPC decoders.

Encoders and decoders are grouped.
Each group contains functions related to a single API, such as Bluetooth or entropy.
A group is created with the :c:macro:`NRF_RPC_GROUP_DEFINE` macro.
Grouping allows you to logically divide the remote API, but also increases performance of nRF RPC.


RPC encoders
************

RPC encoders encode commands and events into serialized packets.
Creating an encoder is similar for all packet types.
To create an encoder, complete the following steps:

1. Allocate a buffer using :c:func:`nrf_rpc_alloc_tx_buf`.
#. Encode parameters directly into the buffer or use the `TinyCBOR`_ library.

The packet is sent using one of the sending functions: :c:func:`nrf_rpc_cmd`, :c:func:`nrf_rpc_cbor_evt`, or similar.

After sending the command, a response is received, so it must be parsed.
There are two ways to parse a response.

The first way is to provide a response handler in the parameters of :c:func:`nrf_rpc_cmd` or :c:func:`nrf_rpc_cbor_cmd`.
It is called before :c:func:`nrf_rpc_cmd` returns.
It can be called from a different thread.

Another way of parsing a response is to call :c:func:`nrf_rpc_cmd_rsp` or :c:func:`nrf_rpc_cbor_cmd_rsp`.
The output of these functions contains the response.
After parsing it, the :c:func:`nrf_rpc_decoding_done` or :c:func:`nrf_rpc_cbor_decoding_done` functions must be called to indicate that parsing is completed and the buffers holding the response can be released.

Events have no response, so they need no additional action after sending them.

The following is a sample command encoder created using the nRF RPC TinyCBOR API.
The function remotely adds ``1`` to the ``input`` parameter and puts the result in the ``output`` parameter.
It returns 0 on success or a negative error code if communication with the remote side failed.

.. code-block:: c

	/* Helper define holding maximum CBOR encoded packet length
	 * for this sample.
	 */
	#define MAX_ENCODED_LEN 16

	/* Defines a group that contains functions implemented in this
	 * sample.
	 */
	NRF_RPC_GROUP_DEFINE(math_group, "sample_math", &transport, NULL, NULL, NULL);

	/* Defines a helper structure to pass the results.
	 */
	struct remote_inc_result {
		int err;
		int output;
	};

	int remote_inc(int input, int *output)
	{
		int err;
		struct remote_inc_result result;
		struct nrf_rpc_cbor_ctx ctx;

		NRF_RPC_CBOR_ALLOC(&math_group, ctx, MAX_ENCODED_LEN);

		cbor_encode_int(&ctx.encoder, input);

		err = nrf_rpc_cbor_cmd(&math_group, MATH_COMMAND_INC, &ctx,
				       remote_inc_rsp, &result);

		if (err == 0) {
			*output = result.output;
			err = result.err;
		}

		return err;
	}

The above code uses the ``remote_inc_rsp`` function to parse the response.
The following code shows how this function might look.

.. code-block:: c

	static void remote_inc_rsp(const struct nrf_rpc_group *group, CborValue *value, void *handler_data)
	{
		CborError cbor_err;
		struct remote_inc_result *result =
			(struct remote_inc_result *)handler_data;

	 	if (!cbor_value_is_integer(value)) {
			result->err = -NRF_EINVAL;
			return;
		}

		cbor_err = cbor_value_get_int(value, &result->output);
		if (cbor_err != CborNoError) {
			result->err = -NRF_EINVAL;
			return;
		}

		result->err = 0;
	}


RPC decoders
************

RPC decoders are registered with macros :c:macro:`NRF_RPC_CMD_DECODER`, :c:macro:`NRF_RPC_CBOR_EVT_DECODER`, or similar, depending on what kind of decoder it is.
Decoders are called automatically when a command or event with a matching ID is received.
Command decoders must send a response.

A RPC decoder associated with the example above can be implemented in the following way:

.. code-block:: c

	/* Defines a group that contains functions implemented in this
	 * sample. Second parameter have to be the same in both remote
	 * and local side.
	 */
	NRF_RPC_GROUP_DEFINE(math_group, "sample_math", &transport, NULL, NULL, NULL);


	static void remote_inc_handler(const struct nrf_rpc_group *group, CborValue *value, void* handler_data)
	{
		int err;
		int input = 0;
		int output;
		struct nrf_rpc_cbor_ctx ctx;

		/* Parsing the input */

	 	if (cbor_value_is_integer(value)) {
			cbor_value_get_int(value, &input);
		}

		nrf_rpc_cbor_decoding_done(group, value);

		/* Actual hard work is done in below line */

		output = input + 1;

		/* Encoding and sending the response */

		NRF_RPC_CBOR_ALLOC(group, ctx, MAX_ENCODED_LEN);

		cbor_encode_int(&ctx.encoder, output);

		err = nrf_rpc_cbor_rsp(group, &ctx);

		if (err < 0) {
			fatal_error(err);
		}
	}

	NRF_RPC_CBOR_CMD_DECODER(math_group, remote_inc_handler,
				 MATH_COMMAND_INC, remote_inc_handler, NULL);
