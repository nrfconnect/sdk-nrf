.. _bt_mesh_loc_readme:

Generic Location models
#######################

The Generic Location models allow remote monitoring of the location status of
a mesh device.

There are two Generic Location models:

- :ref:`bt_mesh_loc_srv_readme`
- :ref:`bt_mesh_loc_cli_readme`

.. _bt_mesh_loc_srv_readme:

Generic Location Server
=======================

The Generic Location Server provides location information about the device.
The location data is split up into two separate states: Global and Local.
The Global Location represents the device location in the world, while the
Local Location represents the device location relative to a local coordinate
system, for instance inside a building.

The Generic Location Server adds two model instances in the composition data:

- The Generic Location Server
- The Generic Location Setup Server

The two model instances share the states of the Generic Location Server, but
accept different messages. This allows fine-grained control of the access
rights for the location states, as the two model instances can be bound to
different application keys.

The Generic Location Server allows observation of the location states, as it
only exposes get-messages for the location states. This is typically the
user facing model instance, as the device location shouldn't normally be
configurable by the users.

The Generic Location Setup Server allows configuration of the location states
by exposing set-messages for the location states. This model instance can be
used to configure the location information of the device, for instance during
deployment.

If the device is capable of determining its own location information, the
Generic Location Setup Server is redundant, and can be deactivated by
removing all bindings it has to any application keys.

.. note::

  The Generic Location Server does not store any of its states persistently,
  so if the Generic Location Server is meant to get its location configured
  during setup, this must be stored by the application.

States
*******

**Global Location**: :cpp:type:`bt_mesh_loc_global`

The Global Location state is a composite state representing a global location
as a WGS84 coordinate point. The latitude and longitude (in degrees) are
represented as a ``double``, and are each encoded as a signed 32 bit integer.
The altitude is represented as a signed 16 bit integer, measured in meters.

The memory for the Global Location state is owned by the user, and should be
exposed to the model through the callbacks in a
:cpp:type:`bt_mesh_loc_srv_handlers` structure.

**Local Location**: :cpp:type:`bt_mesh_loc_local`

The Local Location state represents the device's location within a locally
defined coordinate system, like the inside of a building. It contains
parameters for a three dimensional device location measured in decimeters, as
well as a floor number and parameters to determine the precision of the
location parameters.

The memory for the Local Location state is owned by the user, and should be
exposed to the model through the callbacks in a
:cpp:type:`bt_mesh_loc_srv_handlers` structure.

Extended models
****************

None.

Persistent storage
*******************

None.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_loc_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_loc_srv.c`

.. doxygengroup:: bt_mesh_loc_srv
   :project: nrf
   :members:

----

.. _bt_mesh_loc_cli_readme:

Generic Location Client
=======================

The Generic Location Client model can get and set the state of a Generic
Location Server model remotely.

Contrary to the Server model, the Client only creates a single model instance
in the mesh composition data. The Generic Location Client may send messages to
both the Generic Location Server and the Generic Location Setup Server, as long
as it has the right application keys.

Extended models
****************

None.

Persistent storage
*******************

None.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_loc_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_loc_cli.c`

.. doxygengroup:: bt_mesh_loc_cli
   :project: nrf
   :members:

----

Common types
=============

| Header file: :file:`include/bluetooth/mesh/gen_loc.h`

.. doxygenfile:: gen_loc.h
   :project: nrf
