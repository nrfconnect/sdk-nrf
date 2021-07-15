.. _bt_mesh_battery_srv_readme:

Generic Battery Server
######################

.. contents::
   :local:
   :depth: 2

The Generic Battery Server model provides information about the current battery status of the device.

States
======

The Generic Battery Server model contains the following state:

Generic Battery Status: :c:struct:`bt_mesh_battery_status`
    The Generic Battery Status is a composite state, which means that it contains various information about the battery state.
    The battery state can only be changed locally, so a :ref:`bt_mesh_battery_cli_readme` is only able to observe it.

    Your application is expected to hold the Generic Battery Status state memory and provide access to the state through the :c:member:`bt_mesh_battery_srv.get` handler function.
    All the fields in the Generic Battery Status have special *unknown* values, which are used by default.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_battery_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_battery_srv.c`

.. doxygengroup:: bt_mesh_battery_srv
   :project: nrf
   :members:
