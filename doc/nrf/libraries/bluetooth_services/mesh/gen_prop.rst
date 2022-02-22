.. _bt_mesh_prop_readme:

Generic Property models
#######################

.. contents::
   :local:
   :depth: 2

The Generic Property models allow remote access to the Device Properties of a mesh node.
Read more about Device Properties in :ref:`bt_mesh_properties_readme`.

The Generic Property models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

The following configuration parameters are associated with the Generic Property models:

* :kconfig:option:`CONFIG_BT_MESH_PROP_MAXSIZE` - The largest available property value.
* :kconfig:option:`CONFIG_BT_MESH_PROP_MAXCOUNT` - The largest number of properties available on a single Generic Property Server.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   gen_prop_srv.rst
   gen_prop_cli.rst


Common types
************

This section lists the types common to the Generic Property mesh models.

| Header file: :file:`include/bluetooth/mesh/gen_prop.h`

.. doxygenfile:: gen_prop.h
   :project: nrf
