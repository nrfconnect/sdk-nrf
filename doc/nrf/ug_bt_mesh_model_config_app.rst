.. _ug_bt_mesh_model_config_app:

Configuring mesh models using the nRF Mesh mobile app
#####################################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® mesh :ref:`samples <samples>` use the nRF Mesh mobile app to perform provisioning and configuration.

Binding a model to an application key
*************************************

Mesh models encrypt all their messages using the application keys.
Models that should communicate with each other must be bound to the same application key.

Complete the following steps in the nRF Mesh app to bind a model to an application key:

1. On the Network screen, tap the mesh device node.
   Basic information about the mesh node and its configuration is displayed.
#. In the mesh node view, expand an element.
   It contains the list of models instantiated in the selected element of the node.
#. Tap the model entry to see the model's configuration.
#. Bind the model to application key to make it open for communication:

   a. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select the key from the list.

Creating a group
****************

To send messages to more than one node in the mesh network, you must define groups for the models to publish and subscribe to.
Complete the following steps in the nRF Mesh mobile app to create a new group:

1. On the **Groups** screen, tap :guilabel:`CREATE GROUP`.
#. Ensure that the group address type is selected from the drop-down menu.
#. In the **Name** field, enter the group name.
#. In the **Address** field, enter the group address.
#. Tap :guilabel:`OK`.

Configuring model publication parameters
****************************************

The publication parameters define how the model sends its messages.
To configure the model publication parameters, complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`SET PUBLICATION` under the **Publish** section.
#. Tap :guilabel:`Publish Address` under the **Address** section to set the publish address for the node.
#. Select :guilabel:`Groups` from the drop-down menu.
#. Select an existing group or create a new one.
#. Tap :guilabel:`OK`.
#. If necessary, set the publication interval and publication retransmission configuration.
   Otherwise, leave the publication parameters at their default values.

   a. Scroll down to the **Publish Period** view and set the publication interval by using the **Interval** slider.
      This will make the node publish its presence status periodically at the defined interval.
   b. Change the publication retransmission configuration under the **Publish Retransmission** section. Set the value for the **Retransmit Count**.

#. Tap :guilabel:`APPLY` in the right bottom corner of the screen to confirm the configuration.

Subscribing to a group
**********************

To receive messages sent to group addresses, the models must subscribe to them.
Complete the following steps in the nRF Mesh mobile app to configure subscription parameters:

1. Tap :guilabel:`SUBSCRIBE` under the **Subscriptions** section.
#. Select :guilabel:`Groups`.
#. Select an existing group or create a new one.
#. Tap :guilabel:`OK`.
#. Double-tap the back arrow button at the top left corner of the app to get back to the main application screen.
