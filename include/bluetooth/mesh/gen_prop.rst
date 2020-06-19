.. _bt_mesh_prop_readme:

Generic Property models
#######################

The Generic Property models allow remote access to the Device Properties of a mesh node.
Read more about Device Properties in :ref:`bt_mesh_properties_readme`.

The following Generic Property models are supported:

.. toctree::
   :maxdepth: 1
   :glob:

   gen_prop_srv.rst
   gen_prop_cli.rst

The Generic Property models also feature their own common types, listed in the `Common types`_ section below.
For types common to all models, see :ref:`bt_mesh_models`.

Configuration
=============

The following configuration parameters are associated with the Generic Property models:

* :option:`CONFIG_BT_MESH_PROP_MAXSIZE` - The largest available property value.
* :option:`CONFIG_BT_MESH_PROP_MAXCOUNT` - The largest number of properties available on a single Generic Property Server.

Common types
============

| Header file: :file:`include/bluetooth/mesh/gen_prop.h`

.. doxygenfile:: gen_prop.h
   :project: nrf
