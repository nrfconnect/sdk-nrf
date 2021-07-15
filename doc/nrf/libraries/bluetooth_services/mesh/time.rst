.. _bt_mesh_time_readme:

Time models
###########

.. contents::
   :local:
   :depth: 2

The Time models allow network-wide time and date synchronization with a granularity of 3.9 ms (1/256th second), and provide services for converting timestamps to and from time zone adjusted UTC time (Coordinated Universal Time).
The time is measured based on the International Atomic Time (TAI).

The Time models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   time_tai.rst
   time_srv.rst
   time_cli.rst

Common types
************

This section lists the types common to the Time mesh models.

| Header file: :file:`include/bluetooth/mesh/time.h`

.. doxygengroup:: bt_mesh_time
   :project: nrf
   :members:
