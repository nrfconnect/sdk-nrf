.. _bt_mesh_battery_srv_readme:

Generic Battery Server
######################

The Generic Battery Server model provides information about the current battery status of the device.

States
=======

**Generic Battery Status**: :cpp:type:`bt_mesh_battery_status`

The Generic Battery Status is a composite state, containing various information about the battery state.
The battery state can only be changed locally, so a Generic Battery Client is only able to observe it.

The user is expected to hold the Generic Battery Status state memory and provide access to the state through the :cpp:member:`bt_mesh_battery_srv::get` handler function.
All the fields in the Generic Battery Status have special *unknown* values, which are used by default.

Extended models
================

None.

Persistent storage
===================

None.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/gen_battery_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_battery_srv.c`

.. doxygengroup:: bt_mesh_battery_srv
   :project: nrf
   :members:
