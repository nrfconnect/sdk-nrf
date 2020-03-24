.. _bt_mesh_lvl_srv_readme:

Generic Level Server
####################

The Generic Level Server model owns a single Generic Level state.

States
=======

**Generic Level**: ``s16_t``

The user is expected to hold the state memory and provide access to the state through the :cpp:type:`bt_mesh_lvl_srv_handlers` handler structure.

Changes to the Generic Level state may include transition parameters.
While transitioning to a new level state, any requests to read out the current level should report the actual current level in the transition, as well as the terminal level and remaining time in milliseconds, including delay.

If the transition includes a delay, the state shall remain unchanged until the delay expires.

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
