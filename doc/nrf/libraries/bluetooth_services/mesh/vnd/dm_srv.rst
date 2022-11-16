.. _bt_mesh_dm_srv_readme:

Distance Measurement Server
###########################

.. contents::
   :local:
   :depth: 2

The Distance Measurement Server vendor model provides capabilities to measure distance between Bluetooth® mesh devices within radio proximity.
The measurements are conducted through the :ref:`mod_dm` library.

At creation, each Distance Measurement Server instance must be initialized with a memory object that can hold one or more measurement results.
The Distance Measurement Server populates this memory object whenever a new measurement is conducted.
The results can be queried by a :ref:`bt_mesh_dm_cli_readme`.

.. note::
   The current implementation is classified with :ref:`experimental <software_maturity>` software maturity.

States
======

None

Extended models
===============

None

Persistent storage
==================

The Distance Measurement Server has following runtime configuration options:

* Default time to live for sync messages.
* Default timeout for distance measurement procedure.
* Default reflector start delay.

If the :kconfig:option:`CONFIG_BT_SETTINGS` option is enabled, the Distance Measurement Server stores its configuration states persistently using a configurable storage delay to stagger storing.
See option :kconfig:option:`CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT`.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/vnd/dm_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/vnd/dm_srv.c`

.. doxygengroup:: bt_mesh_dm_srv
   :project: nrf
   :members:
