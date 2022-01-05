.. _bt_mesh_model_main_template:

Mesh model template
###################

.. contents::
   :local:
   :depth: 2

.. tip::
   This is the mesh model overview page.
   On this page, explain what this model allows you to do within the broader context of the mesh network and list server and client models that are part of this mesh model.
   Use max two sentences (full sentences, not sentence fragments) for the explanation.
   You can go into more detail on the model pages.

   Make sure you link to the server and client model pages and include them in the ``.. toctree:`` below by listing their file names.
   To create these pages, use the :ref:`server and client model template<bt_mesh_model_server_client_template>`.

The XYZ mesh models allow to remotely control a disco light with a mesh device.

The XYZ mesh models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   XYZ Server model<mesh_model_server_client_template>


Common types*
*************

.. tip::
   The Common types section is optional.
   Use this section to inform if the model does not have its own common types.
   For example, add `The XYZ models only use native types, and have no common model specific types.`

This section lists the types common to the XYZ mesh models.

| Header file: :file:`path/to/the/header/file`
| Source file: :file:`path/to/the/source/file`

.. tip::
   Provide paths to the header and source files for the types common to this model.
   Include also the doxygen group for the model's common types.
   Some mesh models do not have common types, for example sensor models.
