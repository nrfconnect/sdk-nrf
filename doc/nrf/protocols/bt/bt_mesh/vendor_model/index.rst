.. _ug_bt_mesh_vendor_model:

Creating a new model
####################

You may create your own vendor-specific models for Bluetooth® Mesh.
These will enable your devices to provide custom behavior not covered by the already defined standard models.
The following sections describe the basics of creating new models for the Bluetooth Mesh in |NCS|, based on the concepts and principles defined in :ref:`mesh_concepts` and :ref:`Access API <zephyr:bluetooth_mesh_access>`.
It is recommended that you have read and understood these concepts and principles, before proceeding with the model development.

Implementation of the :ref:`bt_mesh_chat_client_model`, from the :ref:`bt_mesh_chat` sample, is used as an example.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   dev_overview
   chat_sample_walk_through
