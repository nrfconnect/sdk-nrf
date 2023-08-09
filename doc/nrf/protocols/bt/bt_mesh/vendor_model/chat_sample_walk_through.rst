.. _ug_bt_mesh_vendor_chat_sample_walk_through:

Chat sample walk-through
########################

After covering the :ref:`basics <ug_bt_mesh_vendor_model_dev_overview>` of the model implementation, let's take a look at how it works on the example of the :ref:`bt_mesh_chat` sample.
The sample implements a vendor model that is called the :ref:`bt_mesh_chat_client_model`.

The code below shows how Company ID, Model ID and message opcodes for the :ref:`messages <bt_mesh_chat_client_model_messages>` are defined in the Chat Client model:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/include/chat_cli.h
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_1
   :end-before: include_endpoint_chat_cli_rst_1

The next code snippet shows the declaration of the opcode list:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_2
   :end-before: include_endpoint_chat_cli_rst_2

To send :ref:`presence <bt_mesh_chat_client_model_states>` and non-private messages, the Chat Client model uses the model publication context.
This will let the Configuration Client configure an address where the messages will be published to.
Therefore, the model needs to declare :c:struct:`bt_mesh_model_pub`, :c:struct:`net_buf_simple` and a buffer for a message to be published.
In addition to that, the model needs to store a pointer to :c:struct:`bt_mesh_model`, to use the Access API.

All these parameters are unique for each model instance.
Therefore, together with other model specific parameters, they can be enclosed within a structure defining the model context:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/include/chat_cli.h
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_3
   :end-before: include_endpoint_chat_cli_rst_3

To initialize the model publication context and store :c:struct:`bt_mesh_model`, the model uses the :c:member:`bt_mesh_model_cb.init` handler:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_4
   :end-before: include_endpoint_chat_cli_rst_4

The model implements the :c:member:`bt_mesh_model_cb.start` handler to notify a user that the mesh data has been loaded from persistent storage and when the node is provisioned:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_5
   :end-before: include_endpoint_chat_cli_rst_5

The presence value of the model needs to be stored in the persistent storage, so it can be restored at the kit reboot.
The :c:func:`bt_mesh_model_data_store` function is called to store the presence value:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_8
   :end-before: include_endpoint_chat_cli_rst_8

To restore the presence value, the model implements the :c:member:`bt_mesh_model_cb.settings_set` handler:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_3
   :end-before: include_endpoint_chat_cli_rst_3

The stored presence value needs to be reset if the node reset is applied.
This will make the model start as it was just initialized.
This is done through the :c:member:`bt_mesh_model_cb.reset` handler:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_6
   :end-before: include_endpoint_chat_cli_rst_6

.. note::
   The :c:member:`bt_mesh_model_cb.settings_set` handler, and both :c:func:`bt_mesh_model_data_store` calls, are only compiled if :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, making it possible to exclude this functionality if the persistent storage is not enabled.

The following code initializes the model callbacks structure:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_7
   :end-before: include_endpoint_chat_cli_rst_7

At this point, everything is ready for defining an entry for the node composition data.
For the convenience, the initialization of the :c:macro:`BT_MESH_MODEL_VND_CB` macro is wrapped into a `BT_MESH_MODEL_CHAT_CLI` macro, which only requires a pointer to the instance of the model context defined above:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/include/chat_cli.h
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_2
   :end-before: include_endpoint_chat_cli_rst_2

The user data, which stores a pointer to the model context, is initialized through the :c:macro:`BT_MESH_MODEL_USER_DATA` macro.
This is done to ensure that the correct data type is passed to `BT_MESH_MODEL_CHAT_CLI`.
The following code snippet shows how the defined macro is used when defining the node composition data:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/model_handler.c
   :language: c
   :start-after: include_startingpoint_model_handler_rst_1
   :end-before: include_endpoint_model_handler_rst_1

The Chat Client model allows sending of private messages.
This means that only the node with the specified address will receive the message.
This is done by initializing :c:struct:`bt_mesh_msg_ctx` with custom parameters:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_9
   :end-before: include_endpoint_chat_cli_rst_9

Note that it also sets :c:member:`bt_mesh_msg_ctx.send_rel` to ensure that the message is delivered to the destination.

When the Chat Client model receives a private text message, or some of the nodes want to get the current presence of the model, it needs to send an acknowledgment back to the originator of the message.
This is done by calling the :c:func:`bt_mesh_model_send` function, with :c:struct:`bt_mesh_msg_ctx` passed to the message handler:

.. literalinclude:: ../../../../../../samples/bluetooth/mesh/chat/src/chat_cli.c
   :language: c
   :start-after: include_startingpoint_chat_cli_rst_1
   :end-before: include_endpoint_chat_cli_rst_1
