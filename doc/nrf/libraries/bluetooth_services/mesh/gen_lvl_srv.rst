.. _bt_mesh_lvl_srv_readme:

Generic Level Server
####################

.. contents::
   :local:
   :depth: 2

The Generic Level Server model contains a single Generic Level state.

States
=======

The Generic Level Server model contains the following state:

Generic Level: ``int16_t``
    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_lvl_srv_handlers` handler structure.

    Changes to the Generic Level state may include transition parameters.
    While transitioning to a new level, any request to read out the current level should report the following information:

    * Actual current level in the transition
    * Target level
    * Remaining time in milliseconds (including an optional delay)

    If the transition includes a delay, the state must remain unchanged until the delay expires.

Extended models
================

None.

Persistent storage
===================

None.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/gen_lvl_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_lvl_srv.c`

.. doxygengroup:: bt_mesh_lvl_srv
   :project: nrf
   :members:
