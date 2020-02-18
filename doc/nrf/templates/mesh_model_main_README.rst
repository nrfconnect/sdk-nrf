.. _bt_mesh_model_main_template:

Mesh model template
###################

.. tip::
   This is the mesh model overview page.
   On this page, explain what this model allows you to do within the broader context of the mesh network and list server and client models that are part of this Mesh model.
   Use max two sentences (full sentences, not sentence fragments) for the explanation.
   You can go into more detail on the model pages.

   Make sure you link to the server and client model pages and include them in the ``.. toctree:`` below by listing their file names.
   To create these pages, use the :ref:`server and client model template<bt_mesh_model_server_client_template>`.

The XYZ Mesh models allow to remotely control a disco light with a mesh device.

There are the following XYZ models:

.. toctree::
   :maxdepth: 1
   :glob:

   XYZ Server model<mesh_model_server_client_template>
   XYZ Client model<mesh_model_server_client_template>

The XYZ Mesh models also feature their own common types, listed in the section below.
For types common to all models, see :ref:`bt_mesh_models`.


Common types*
*************

.. tip::
   The Common types section is optional.
   If the model does not have its own common types, do not add this section, but still mention at the end of the page that the model does not have its own common types.

This section lists the types common to the XYZ mesh model.

| Header file: :file:`path/to/the/header/file`
| Source file: :file:`path/to/the/source/file`

.. tip::
   Provide paths to the header and source files for the types common to this model.
   Include also the doxygen group for the model's common types.
   Some mesh models do not have common types, for example sensor models.
