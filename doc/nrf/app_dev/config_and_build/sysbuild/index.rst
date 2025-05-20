.. _configuring_sysbuild:

Configuring sysbuild
####################

The |NCS| supports Zephyr's :ref:`sysbuild (System Build) <configuration_system_overview_sysbuild>`.

.. ncs-include:: build/sysbuild/index.rst
   :docset: zephyr
   :start-after: #######################
   :end-before: Definitions

.. include:: /app_dev/config_and_build/config_and_build_system.rst
   :start-after: differently than in Zephyr.
   :end-before: Moreover, this |NCS|

For more information, see :ref:`sysbuild_enabled_ncs` and :ref:`sysbuild_enabled_ncs_configuring`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   sysbuild_configuring_west
   sysbuild_images
   zephyr_samples_sysbuild
   sysbuild_forced_options
