.. _bt_mesh_scene_srv_readme:

Scene Server
############

.. contents::
   :local:
   :depth: 2

The Scene Server model holds a list of scenes, and exposes functionality for storing and recalling the state of these scenes.

The Scene Server model must be instantiated in every node in the network that participates in scenes.
A Scene Server model instance stores the scene data for every model on its own element, and every subsequent element until the next Scene Server.

The Scene Server model adds two model instances in the composition data:

* Scene Server
* Scene Setup Server

The two model instances share the states of the Scene Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Scene Server states, as the two model instances can be bound to different application keys.

* Scene Server provides functionality for listing and recalling stored scenes.
* Scene Setup Server provides functionality for storing and deleting scenes.

Operation
=========

The Scene Server stores all scene data persistently using the :ref:`zephyr:settings_api` subsystem.
Every scene is stored as a serialized concatenation of each registered model's state, and only exists in RAM during storing and loading.

It's up to the individual model implementation to correctly serialize and deserialize its state from scene data when prompted.

Models with scene data
**********************

The following models will automatically register as scene entries:

* :ref:`bt_mesh_onoff_srv_readme`
* :ref:`bt_mesh_lvl_srv_readme`
* :ref:`bt_mesh_light_ctl_srv_readme`
* :ref:`bt_mesh_light_ctrl_srv_readme`

All models that extend these models will automatically become part of the scenes as well.

Adding scene data for a model
*****************************

Every model that needs to store its state in scenes must register a :cpp:type:`bt_mesh_scene_entry` structure containing callbacks for storing and recalling the scene data.
The scene entry must be registered with a valid :cpp:type:`bt_mesh_scene_entry_type` structure, which defines the store and recall behavior of the scene entry.

For instance, the Generic OnOff Server model defines its scene entry by adding a `bt_mesh_scene_entry` structure to every :cpp:type:`bt_mesh_onoff_srv`, and registering it with a common type:

.. literalinclude:: ../../../subsys/bluetooth/mesh/gen_onoff_srv.c
   :language: c
   :start-after: include_startingpoint_scene_srv_rst_1
   :end-before: include_endpoint_scene_srv_rst_1

The Generic OnOff Server model only needs to store its OnOff state in the scene, so it always stores and recalls a single byte.
The ``restore`` call contains a transition time pointer, which should be taken into account when implementing the model behavior.

If a scene store operation is requested in the middle of a transition between two states, the model should commit the target state to the scene data.

Invalidating scene data
***********************

A scene is valid as long as its state is unchanged.
As soon as one of the models in a scene detect a change to the scene data, either because of an incoming message or because of an internal change, the node is no longer in an active scene.
When a model detects a change to its own data, it shall call :cpp:func:`bt_mesh_scene_invalidate`, passing its own scene entry.
This resets the Scene Server's current scene to :c:macro:`BT_MESH_SCENE_NONE`.
The scene has to be stored again to include the change.

States
======

Current scene: ``uint16_t``
   The currently active scene.
   As soon as one of the models included in the scene changes its state, the scene is invalidated, and set to :c:macro:`BT_MESH_SCENE_NONE`.

Scene registry: ``uint16_t[]``
   The full list of scenes stored by the Scene Server.
   The scene registry has a limited number of slots, controlled by :option:`CONFIG_BT_MESH_SCENES_MAX`.

Extended models
================

None.

Persistent storage
===================

The Scene Server stores the scene registry and the current scene persistently.

Each scene in the scene registry is stored as a separate serialized data structure, containing the scene data of all participating models.
The serialized data is split into pages of 256 bytes to allow storage of more data than the settings backend can fit in one entry.

The serialized scene data includes 4 bytes of overhead for every stored SIG model, and 6 bytes of overhead for every stored vendor model.

.. note::

   As the Scene Server will store data for every model for every scene, the persistent storage space required for the Scene Server is significant.
   It's important to monitor the storage requirements actively during development to ensure the allocated flash pages aren't worn out too early.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/scene_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/scene_srv.c`

.. doxygengroup:: bt_mesh_scene_srv
   :project: nrf
   :members:
