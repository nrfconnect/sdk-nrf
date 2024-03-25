.. _multicore_hello_world:

Multicore Hello World application
#################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to build a Hello World application that runs on multiple cores.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how to build a multicore Hello World application with the :ref:`zephyr:sysbuild`.
When building with Zephyr Sysbuild, the build system adds child images based on the options selected in the project's additional configuration and build files.
This sample shows how to inform the build system about dedicated sources for additional images.
The sample comes with the following additional files:

* :file:`Kconfig.sysbuild` - This file is used to add Sysbuild configuration that is passed to all the images.
  ``SB_CONFIG`` is the prefix for sysbuild's Kconfig options.
* :file:`sysbuild.cmake` - The CMake file adds additional images using the :c:macro:`ExternalZephyrProject_Add` macro.
  You can also add the dependencies for the images if required.

Both the application and remote cores use the same :file:`main.c` that prints the name of the DK on which the application is programmed.

Building and running
********************

.. |sample path| replace:: :file:`samples/multicore/hello_world`

.. include:: /includes/build_and_run_sb.txt

The remote board needs to be specified using ``SB_CONFIG_REMOTE_BOARD``.
As shown below, it is recommended to use configuration setups from :file:`sample.yaml` using the ``-T`` option to build the sample.

nRF5340 DK
  You can build the sample for application and network cores as follows:

  .. code-block:: console

     west build -p -b nrf5340dk/nrf5340/cpuapp -T sample.multicore.hello_world.nrf5340dk_cpuapp_cpunet .

nRF54H20 DK
  You can build the sample for application and radio cores as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.multicore.hello_world.nrf54h20dk_cpuapp_cpurad .

  You can build the sample for application and PPR cores as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.multicore.hello_world.nrf54h20dk_cpuapp_cpuppr .

  Note that :ref:`zephyr:nordic-ppr` is used in the configuration above to automatically launch PPR core from the application core.

  An additional configuration setup is provided to execute code directly from MRAM on the PPR core.
  This configuration uses :ref:`zephyr:nordic-ppr-xip` and enables :kconfig:option:`CONFIG_XIP` on the PPR core.
  It can be built as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.multicore.hello_world.nrf54h20dk_cpuapp_cpuppr_xip .

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

      1. |connect_terminal|
      #. Reset the kit.
      #. Observe the console output for both cores:

         * For the application core, the output should be as follows:

            .. code-block:: console

               *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
               Hello world from nrf5340dk/nrf5340/cpuapp
               Hello world from nrf5340dk/nrf5340/cpuapp
               ...

         * For the remote core, the output should be as follows:

            .. code-block:: console

               *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
               Hello world from nrf5340dk/nrf5340/cpunet
               Hello world from nrf5340dk/nrf5340/cpunet
               ...
