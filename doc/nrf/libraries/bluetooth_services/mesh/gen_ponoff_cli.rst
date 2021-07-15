.. _bt_mesh_ponoff_cli_readme:

Generic Power OnOff Client
##########################

.. contents::
   :local:
   :depth: 2

The Generic Power OnOff Client model remotely controls the state of a :ref:`bt_mesh_ponoff_srv_readme` model.

Unlike the Generic Power OnOff Server, the Generic Power OnOff Client only adds one model instance to the composition data.
The Generic Power OnOff Client may send messages to both the Generic Power OnOff Server and the Generic Power OnOff Setup server, as long as it has the right application keys.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_ponoff_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_ponoff_cli.c`

.. doxygengroup:: bt_mesh_ponoff_cli
   :project: nrf
   :members:
