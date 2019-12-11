.. _bt_mesh_models:

Bluetooth Mesh Models
######################

Nordic Semiconductor provides a variety of model implementations from the Mesh
Model Specification.

Here you can find documentation for these model implementations, including API
documentation.

.. toctree::
   :maxdepth: 1
   :caption: Model implementations:
   :glob:

   ../../../include/bluetooth/mesh/gen_*
   ../../../include/bluetooth/mesh/lightness.rst

Common model types
===================

Models in the Bluetooth Mesh Model Specification share some common types, that
are collected in a single header file.

The following types are common for all models:

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/model_types.h`

.. doxygengroup:: bt_mesh_model_types
   :project: nrf
   :members:
