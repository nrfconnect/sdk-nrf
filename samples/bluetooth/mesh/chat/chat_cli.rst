.. _bt_mesh_chat_client_model:

Chat Client model
#################

.. contents::
   :local:
   :depth: 2

The Chat Client model is a vendor model that allows communication with other such models, by sending text messages and providing the presence of the model instance.
It demonstrates basics of a vendor model implementation.
The model does not have a limitation on per-node instantiations of the model, and therefore can be instantiated on each element of the node.

Overview
********

In this section, you can find more detailed information about the following aspects of the Chat Client:

* `Composition data structure`_
* `Messages`_

.. _bt_mesh_chat_client_model_composition:

Composition data structure
==========================

The Chat Client model is a vendor model, and therefore in the application, when defining the node composition data, it needs to be declared in the third argument in the :c:macro:`BT_MESH_ELEM` macro:

.. code-block:: c

    static struct bt_mesh_elem elements[] = {
        BT_MESH_ELEM(1,
            BT_MESH_MODEL_LIST(
                BT_MESH_MODEL_CFG_SRV(&cfg_srv),
                BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
            BT_MESH_MODEL_LIST(
                BT_MESH_MODEL_CHAT_CLI(&chat))),
    };

.. _bt_mesh_chat_client_model_messages:

Messages
========

The Chat Client model defines the following messages:

Presence
   Used to report the current model presence.
   When the model periodic publication is configured, the Chat Client model will publish its current presence, regardless of whether it has been changed or not.
   Presence message has a defined length of 1 byte.

Presence Get
   Used to retrieve the current model presence.
   Upon receiving the Presence Get message, the Chat Client model will send the Presence message with the current model presence stored in the response.
   The message does not have any payload.

Message
   Used to send a non-private text message.
   The payload consists of the text string terminated by ``\0``.
   The length of the text string can be configured at the compile-time using :ref:`CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH <CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH>` option.

Private Message
   Used to send a private text message.
   When the model receives this message, it replies with the Message Reply.
   The payload consists of the text string terminated by ``\0``.
   The length of the text string can be configured at the compile-time using :ref:`CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH <CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH>` option.

Message Reply
   Used to reply on the received Private Message to confirm the reception.
   The message does not have any payload.

Configuration
*************
|config|

Configuration options
=====================

The following configuration parameters are associated with the Chat Client model:

.. _CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH:

CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH - Message length configuration
   Maximum length of the message to be sent over the mesh network.

.. _bt_mesh_chat_client_model_states:

States
******

The Chat Client model contains the following states:

Presence: ``bt_mesh_chat_cli_presence``:
    The Chat Client model enables a user to set a current presence of the client instantiated on the element of the node.
    It can have the following values:

    * :c:enumerator:`BT_MESH_CHAT_CLI_PRESENCE_AVAILABLE` - The client is available.
    * :c:enumerator:`BT_MESH_CHAT_CLI_PRESENCE_AWAY` - The client is away.
    * :c:enumerator:`BT_MESH_CHAT_CLI_PRESENCE_INACTIVE` - The client is inactive.
    * :c:enumerator:`BT_MESH_CHAT_CLI_PRESENCE_DO_NOT_DISTURB` - The client is in "do not disturb" state.

Extended models
***************

None.

Persistent storage
******************

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Chat Client stores its presence state.
