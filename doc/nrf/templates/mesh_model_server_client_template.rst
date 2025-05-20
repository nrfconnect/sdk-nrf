.. _bt_mesh_model_server_client_template:

Mesh server and client model template
#####################################

.. contents::
   :local:
   :depth: 2

.. tip::
   Use this template to create pages that describe the server or the client models of the related mesh model, or both.
   To create the main mesh model page, use the :ref:`bt_mesh_model_main_template`.
   Make sure the server and client model pages are nested under this main mesh model page under the ``.. toctree:`` tag.

.. note::
   Sections with * are optional.

The XYZ Server/XYZ Client provides information about the disco light.
The data is split into the following states: location and color.

The XYZ Server adds the following model instances in the composition data:

* XYZ Server
* XYZ Setup Server

The XYZ Server allows observation of the location states, as it only exposes get-messages for the location states.

.. tip::
   This is an overview section.
   When describing a server model:

   * Start with a general purpose of the server model.
   * Provide basic information about the states, if the server model has any.
   * List the model instances of the server model (if there are more than one).
     Say what they allow in relation to states and to each other, describe usage criteria and corner cases.

   When describing a client model:

   * Describe the client model role and purpose in relation to the server model.
   * Clearly explain the differences in operation of the client model as compared with the server model.

   Because of the character of the client models, the description for client models will be shorter than the one for server models.

Configuration*
**************

The following configuration parameters are associated with the XYZ Server/XYZ Client model:

* ``:kconfig:PARAM_1`` - Short description of the first parameter.
* ``:kconfig:PARAM_2`` - Short description of the second parameter.

.. tip::
   List Kconfig options associated with the model, and link to them using the `:kconfig:option:` reference.
   This section is optional, because not all models have Kconfig parameters that allow configuration.
   Do not go into details here, as the link will allow the reader to get the required information.
   However, do provide a short description of each option.

States
******

.. note::
   This section is valid for server models only.

The XYZ Server model contains the following states:

Generic Power On: `bt_mesh_template_location_state`
    The Generic Power On state controls the default value of the disco light when the device powers up.
    It can have the following values:

    * `Value_1` - The disco light powers up and starts blinking.
    * `Value_2` - The disco light powers up and starts with solid lighting.

    Changes to the state should be exposed to the model through the XYZ callback.

.. tip::
   Describe each state using the definition list format (with header and indented definition).
   Mention when it is used and why.
   Provide information about variables, parameters, and values of the state.
   If needed, describe the callback used for exposing the changes to the model.

State transition patterns*
==========================

See the following figure for the breakdown of the state transition patterns.

.. tip::
   If needed, visualize the transition patterns with an SVG image or a table.

Extended models
***************

.. tip::
   Not all models have extended models.
   If a model does not have any extended model, write *None.* in this section.

The XYZ Server/XYZ Client extends the following models:

* Model 1
* Model 2

The Generic Power On state is bound to the Generic OnOff state of the Model 1 through its power up behavior.
No other state bindings are present, and the callbacks are forwarded to the application as they are.

.. tip::
   List the models that are extended by the model you are describing, and link to these models.
   A model that extends another inherits the model's functionality and gives context to its states and messages.
   The description that follows the list describes how the extended models are handled and how they relate.

Persistent storage
******************

.. tip::
   Not all models store information persistently.
   If a model does not use persistent storage, write *None.* in this section.

The information about the Generic Power On state is stored persistently.

.. tip::
   Describe what information is stored persistently.
   You can also specify where it is stored and what it is used for.


API documentation
*****************

| Header file: :file:`path/to/the/header/file`
| Source file: :file:`path/to/the/source/file`

.. tip::
   Provide paths to the header and source files of the server model API.
   Include also the doxygen group for the server model you are describing.
