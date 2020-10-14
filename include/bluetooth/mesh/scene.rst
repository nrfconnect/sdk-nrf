.. _bt_mesh_scene_readme:

Scene models
############

.. contents::
   :local:
   :depth: 2

The Scene models allow storing the model state of the entire mesh network as a *scene*, which can be recalled at a later time.
The Scene models are most commonly used to implement presets for different times and activities.
Scenes typically show up in the end user application as user presets, like "Dinner lights", "Night time" or "Movie night".

The Scene models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   scene_srv.rst
   scene_cli.rst

Common types
============

This section lists the types common to the Scene mesh models.

| Header file: :file:`include/bluetooth/mesh/scene.h`

.. doxygengroup:: bt_mesh_scene
   :project: nrf
   :members:
