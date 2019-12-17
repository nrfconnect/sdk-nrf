.. _bt_mesh_prop_readme:

Generic Property models
#######################

The Generic Property models allow remote access to the Device Properties of a
mesh node. Read more about device properties in
:ref:`bt_mesh_properties_readme`.

The Generic Property models fall into two categories:

- :ref:`bt_mesh_prop_srv_readme`
- :ref:`bt_mesh_prop_cli_readme`

Configuration
==============

There are two configuration parameters associated with the Generic Property
models:

- :option:`CONFIG_BT_MESH_PROP_MAXSIZE`: The largest available property value.
- :option:`CONFIG_BT_MESH_PROP_MAXCOUNT`: The largest number of properties
  available on a single Generic Property Server.

.. _bt_mesh_prop_srv_readme:

Generic Property Servers
========================

A Generic Property Server holds a list of Device Properties.
There are four different property servers:

- **Generic Manufacturer Property Server**: Provides access to Manufacturer
  properties, which are read-only properties set by the device manufacturer.
- **Generic Admin Property Server**: Provides access to Admin properties, which
  can be read and written to.
- **Generic User Property Server**: Provides access to both Admin and
  Manufacturer properties on its element, but only if the Property owner allows
  it.
- **Generic Client Property Server**: Provides a list of all properties
  supported by the Generic Property Client on the same element. The Client
  Property Server does not actually own any properties, and does not need the
  user to provide access to any property values.

Typically, only the administrator of the mesh network should have access to the
Admin or Manufacturer servers, all other mesh nodes should operate on the User
server. The Manufacturer and Admin servers may change the User property access
rights for each individual Property it owns at runtime.

The Manufacturer and Admin Servers must provide a list of all Properties they
own in the initialization of the model. Access to the property values should be
provided through the server getter (:cpp:member:`bt_mesh_prop_srv::get`) and
setter (:cpp:member:`bt_mesh_prop_srv::set`) callbacks.

The User Property server does not own any properties, but relies on Admin and
Manufacturer servers on the same element to provide access to theirs. An
element with a User Property Server and no Admin or Manufacturer Servers is
useless.

The model keeps the user access field of each Admin and Manufacturer property
in persistent storage. The property values themselves have to be stored by the
application.

The set of Property IDs and their order cannot be changed.

States
*******

**Device Properties**: :cpp:type:`bt_mesh_prop`

A single server may own several Device Properties, which are accessed by their
Property ID. Each Property holds a value and a user access parameter, which
controls the Property's availability to a Generic User Property Server on the
same element.

The Device properties have slightly different access restrictions depending on
the type of server that owns them:

- **Manufacturer Property**: The value can only be read.
- **Admin Property**: The value can be read and written.
- **Client Property**: Has no associated value, and is only a listing of the
  property IDs supported by the accompanying Generic Property Client.

Extended models
****************

The Generic Admin Property Server and Generic Manufacturer Property Server should both extend the Generic User Property Server.
This is not handled automatically by the model implementation.
For this reason, a Generic User Property Server must be instantiated in the same element in the device composition data as the Generic Admin or Generic Manufacturer Property server.
This ensures compliance with the Bluetooth Mesh Model specification.

Persistent storage
*******************

The Generic Manufacturer Property Server and Generic Admin Property Server
models will store changes to the user access rights of their properties.
Any permanent changes to the property values themselves should be stored
manually by the application.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_prop_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_srv.c`

.. doxygengroup:: bt_mesh_prop_srv
   :project: nrf
   :members:

----

.. _bt_mesh_prop_cli_readme:

Generic Property Client
=======================

The Generic Property Client model can access properties from a Property Server
remotely. The Property Client can talk directly to all types of Property
Servers, but only if it shares an application key with the target server.

Generally, the Property Client should only target User Property Servers, unless
it is part of some network administrator node that is responsible for
configuring the other mesh nodes.

To ease configuration, the Property Client can be paired with a Client Property
Server that lists which properties this Client will request.

Extended models
****************

None.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_prop_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_cli.c`

.. doxygengroup:: bt_mesh_prop_cli
   :project: nrf
   :members:

----

Common types
=============

| Header file: :file:`include/bluetooth/mesh/gen_prop.h`

.. doxygenfile:: gen_prop.h
   :project: nrf
