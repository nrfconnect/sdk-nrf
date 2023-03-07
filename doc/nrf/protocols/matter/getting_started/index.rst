.. _ug_matter_intro_gs:

Getting started with Matter
###########################

.. contents::
   :local:
   :depth: 2

Read pages in this section to start working with Matter using Nordic Semiconductor's SoCs and tools in both the |NCS| and Zephyr.

The pages will guide you through the following getting started process:

1. In :ref:`ug_matter_hw_requirements`, you will learn about Nordic Semiconductor's SoCs that are compatible with Matter and the RAM and flash memory requirements for each of the :ref:`matter_samples`.
#. :ref:`ug_matter_gs_testing` will guide you through the process of setting up the development environment for Matter.
   Several options are available based on your choice of the IPv6 network and the Matter controller type.
   During this process, you will have to program a Matter sample.
   We recommend using :ref:`Matter light bulb <matter_light_bulb_sample>`.
   You will also use some of the :ref:`ug_matter_tools` for this process.
#. :ref:`ug_matter_gs_kconfig` describes the basic Kconfig configuration if you want to start developing your own Matter application.
#. In :ref:`ug_matter_device_advanced_kconfigs`, you will learn about more advanced Kconfig options related to Matter.
#. :ref:`ug_matter_gs_transmission_power` provides information about the default transmission values for different protocols related to Matter and the list of Kconfig options that you can used to modify the transmission power.
#. Once you have a grasp of the Matter configuration, :ref:`ug_matter_gs_adding_cluster` will teach you step by step how to add clusters to a Matter application created from the template sample.
#. Finally, in :ref:`ug_matter_gs_ecosystem_compatibility_testing`, you will set up and test multiple Matter fabrics, each belonging to a different commercial ecosystem, and test their interoperability.

Some of the pages will make reference to external documentation pages available in the |NCS| documentation under the :ref:`matter_index` tab.
These are built from the files available in the official `Matter GitHub repository`_ and refer to the nRF Connect platform.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   hw_requirements
   testing/index
   tools
   kconfig
   advanced_kconfigs
   low_power_configuration
   transmission_power
   adding_clusters
   ecosystem_compatibility_testing
