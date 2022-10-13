.. _ug_matter_overview_int_model:

Matter Interaction Model and interaction types
##############################################

.. contents::
   :local:
   :depth: 2

.. ug_matter_int_model_desc_start

The Interaction Model layer defines what interactions can be performed between a client and a server device.
The node that initiates the interaction is called initiator (typically, a client device), and the node that is the destinatary of the interaction is called target (typically, a server device).

.. ug_matter_int_model_desc_end

The following interaction types belong to the Interaction Model:

* Read - This interaction is used get the value of attributes or events.
* Write - This interaction is used to modify attribute values.
* Invoke - This interaction is used to send commands.
* Subscribe - This interaction is used to create subscription with a target in order to receive data reports from the target periodically instead of polling for data.
  Subscriptions can be related to attributes and events.

Each interaction is made of transactions, which in turn are made of actions.
Each action can be conveyed by one or more messages.
