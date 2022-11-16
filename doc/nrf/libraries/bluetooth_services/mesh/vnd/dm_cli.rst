.. _bt_mesh_dm_cli_readme:

Distance Measurement Client
###########################

.. contents::
   :local:
   :depth: 2

The Distance Measurement Client vendor model remotely controls the state of a :ref:`bt_mesh_dm_srv_readme` model.

At creation, each Distance Measurement client instance must be initialized with a memory object that can hold one or more measurement results.

.. note::
   The current implementation is classified with :ref:`experimental <software_maturity>` software maturity.

Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/vnd/dm_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/vnd/dm_cli.c`

.. doxygengroup:: bt_mesh_dm_cli
   :project: nrf
   :members:
