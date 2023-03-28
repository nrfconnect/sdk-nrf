.. _multicore_hello_world:

Multicore Hello World application
#################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to build the Hello World application for the multicore SoC.
The sample adds a remote image as a child image from the sample source files.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how to build a multicore Hello World application with the :c:macro:`add_child_image` CMake macro.
For general information about multi-image builds, see :ref:`ug_multi_image`.
When building any multi-image application, the build system in the |NCS| adds the child images based on the options selected for the parent image.
This sample shows how to inform the build system about dedicated sources for the child image (in the :file:`zephyr` and :file:`aci` directories).
The sample comes with the following additional files:

* :file:`aci/CMakeLists.txt` - This file is used to add the child image and contains the :c:macro:`add_child_image` macro.
* :file:`zephyr/module.yml` - This file contains information about the location of the :file:`CMakeLists.txt` file that invokes :c:macro:`add_child_image` that is required for the build system.
* :file:`Kconfig` - Custom file that allows to add child image only for the application core.
  Additionally, it enables options required by the multi-image build.

Both the application and remote cores use the same :file:`main.c` that prints the name of the DK on which the application is programmed.

Building and running
********************
.. |sample path| replace:: :file:`samples/multicore/hello_world`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

.. tabs::
   .. tab:: nRF5340

      1. |connect_terminal|
      #. Reset the kit.
      #. Observe the console output for both cores:

         * For the application core, the output should be as follows:

            .. code-block:: console

               *** Booting Zephyr OS build v2.7.99-ncs1-2193-gd359a86abf14  ***
               Hello world from nrf5340dk_nrf5340_cpuapp

         * For the network core, the output should be as follows:

            .. code-block:: console

               *** Booting Zephyr OS build v2.7.99-ncs1-2193-gd359a86abf14  ***
               Hello world from nrf5340dk_nrf5340_cpunet
