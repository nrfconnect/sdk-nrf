.. _bt_mesh_prop_cli_readme:

Generic Property Client
#######################

.. contents::
   :local:
   :depth: 2

The Generic Property Client model can access properties from a :ref:`Property Server <bt_mesh_prop_srv_readme>` remotely.
The Property Client can talk directly to all types of Property Servers, but only if it shares an application key with the target server.

Generally, the Property Client should only target User Property Servers, unless it is part of a network administrator node that is responsible for configuring the other mesh nodes.

To ease the configuration, the Property Client can be paired with a Client Property Server that lists which properties this Property Client will request.

Extended models
===============

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_prop_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_cli.c`

.. doxygengroup:: bt_mesh_prop_cli
   :project: nrf
   :members:
