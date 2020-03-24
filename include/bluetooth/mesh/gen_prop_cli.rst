.. _bt_mesh_prop_cli_readme:

Generic Property Client
#######################

The Generic Property Client model can access properties from a Property Server remotely.
The Property Client can talk directly to all types of Property Servers, but only if it shares an application key with the target server.

Generally, the Property Client should only target User Property Servers, unless it is part of some network administrator node that is responsible for configuring the other mesh nodes.

To ease configuration, the Property Client can be paired with a Client Property Server that lists which properties this Client will request.

Extended models
================

None.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/gen_prop_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_cli.c`

.. doxygengroup:: bt_mesh_prop_cli
   :project: nrf
   :members:
