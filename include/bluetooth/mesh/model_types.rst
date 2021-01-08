.. _bt_mesh_models_overview:

Bluetooth mesh models overview
##############################

.. contents::
   :local:
   :depth: 2

A Bluetooth mesh model is a standardized component that defines a series of states and related behaviors.
Models encapsulate a single feature of a mesh node, and expose this feature to the mesh network.
Each mesh-based product implements several models.

States, either simple or complex, are used to indicate the condition of a device, for example whether it is on or off.
Some states are bound to other, which causes them to mutually update their values.

.. _bt_mesh_models_categorization:

Categorization
**************

Each model is classified into one of the following categories:

* Server -- which contains states.
* Client -- which reads and writes the the Server's states.

Moreover, some models also include a *Setup Server* model instance.
The two server model instances share the states of the server model, but accept different messages.
This allows for a fine-grained control of the access rights for the states, as the two model instances can be bound to different application keys.
Typically, the Setup Server instance provides write access to configuration parameters, such as valid parameter ranges or default values.

Both server and client models can be extended, but because client models do not have states, there is generally no reason to extend them.
None of the specification client models extend other models.
All server models store changes to their configuration persistently using the :ref:`zephyr:settings_api` subsystem.

.. _bt_mesh_models_configuration:

Configuration
*************

You can configure mesh models in |NCS| using Kconfig options.
See :ref:`configure_application` for more information.

The options related to each model configuration are listed in the respective documentation pages.

.. _bt_mesh_models_common_types:

Common types for all models
***************************

All models in the Bluetooth mesh model specification share some common types, which are collected in a single header file.
These include transitions, common IDs and other types that are rarely actively used.

Each model can also contain its own common types, listed in the respective documentation pages.
For the types common to all models, see the following section.

The following types are common for all models:

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/model_types.h`

.. doxygengroup:: bt_mesh_model_types
   :project: nrf
   :members:
