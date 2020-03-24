.. _bt_mesh_prop_readme:

Generic Property models
#######################

The Generic Property models allow remote access to the Device Properties of a mesh node.
Read more about device properties in :ref:`bt_mesh_properties_readme`.

The Generic Property models fall into two categories:

.. toctree::
   :maxdepth: 1
   :glob:

   gen_prop_srv.rst
   gen_prop_cli.rst

Configuration
==============

There are two configuration parameters associated with the Generic Property models:

- :option:`CONFIG_BT_MESH_PROP_MAXSIZE`: The largest available property value.
- :option:`CONFIG_BT_MESH_PROP_MAXCOUNT`: The largest number of properties available on a single Generic Property Server.

Common types
=============

| Header file: :file:`include/bluetooth/mesh/gen_prop.h`

.. doxygenfile:: gen_prop.h
   :project: nrf
