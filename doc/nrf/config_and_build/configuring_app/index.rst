.. _gs_modifying:
.. _configure_application:

Configuring and building an application
#######################################

As mentioned on the :ref:`app_build_system` page, the build and configuration system in the |NCS| uses the following building blocks as a foundation:

.. include:: /config_and_build/config_and_build_system.rst
   :start-after: as a foundation:
   :end-before: Each of these systems

This section provides information about how to change the |NCS| configuration for each of the building blocks inherited from Zephyr.
(The :ref:`partition_manager` is described in the Scripts section).

Moreover, this section describes different aspects of the :ref:`sysbuild configuration <configuring_sysbuild>` in the |NCS|.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   cmake/index
   hardware/index
   kconfig/index
   sysbuild/index
   building
   output_build_files
