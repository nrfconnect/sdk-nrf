.. _ug_nrf54h20_ppr:

Working with the PPR core
#########################

.. contents::
   :local:
   :depth: 2

The nRF54H20 SoC includes a dedicated VPR CPU, based on RISC-V architecture, known as the *Peripheral Processor* (PPR).

.. _vpr_ppr_nrf54h20_initiating:

Using Zephyr multithreaded mode on PPR
**************************************

The PPR core can operate as a general-purpose core, running under the full Zephyr kernel.
Building the PPR target is similar to building the application core, but the application core build must include an overlay that enables the PPR core.

Bootstrapping the PPR core
==========================

The |NCS| provides a PPR snippet that adds the overlay needed for bootstrapping the PPR core.
The primary purpose of this snippet is to enable the transfer of the PPR code to the designated memory region (if required) and to initiate the PPR core.

When building for the ``nrf54h20dk/nrf54h20/cpuppr`` target, a minimal sample is automatically loaded onto the application core.
For more details, see :ref:`building_nrf54h_app_ppr_core`.

Memory allocation
*****************

Running the PPR CPU can lead to increased latency when accessing ``RAM_30``.
To mitigate this, use ``RAM_30`` exclusively for PPR code, PPR data, and non-time-sensitive data from the application CPU.
If both ``RAM_30`` and ``RAM_31`` are available, prefer using ``RAM_31`` to avoid memory access latency caused by the PPR.
For data that requires strict access times, such as CPU data used in low-latency Interrupt Service Routines (ISRs), use local RAM, or ``RAM_0x`` when higher latency is acceptable.
Place the DMA buffers in a memory designed to a given peripheral.

Building and programming with the nRF54H20 DK
*********************************************

Depending on the sample, you might need to program only the application core or both the PPR and application cores.
Additionally, the process varies depending on whether you are working with a single-image or multi-image build.

.. note::
   The following instructions do not cover the scenario of multi-image single-core builds.

Building for the application core only
======================================

Building for the application core follows the default building process for the |NCS|.
For detailed instructions, refer to the :ref:`building` page.

.. _building_nrf54h_app_ppr_core:

Building for both the application and PPR core
===============================================

Building for both the application core and the PPR core differs from the default |NCS| procedure.
Additional configuration is required to enable the PPR core.

This section explains how to build and program both cores, covering separate builds and sysbuild configurations.
The PPR core supports two variants:

* ``nrf54h20dk/nrf54h20/cpuppr``: PPR runs from ``RAM_30`` (recommended method).
  The application core image must include the ``nordic-ppr`` :ref:`snippet <app_build_snippets>`.

* ``nrf54h20dk/nrf54h20/cpuppr/xip``: PPR runs from MRAM.
  The application core image must include the ``nordic-ppr-xip`` snippet.

Standard build
--------------

This section explains how to build an application using :ref:`sysbuild <configuration_system_overview_sysbuild>`.

.. note::
   Currently, the documentation does not provide specific instructions for building an application image using sysbuild to incorporate the PPR core as a sub-image.
   The only documented scenario involves building the PPR as the main image and the application as a sub-image.

To complete the build, do the following:

.. tabs::

   .. group-tab:: Using minimal sample for VPR bootstrapping

      This option automatically programs the PPR core with :ref:`dedicated bootstrapping firmware <vpr_ppr_nrf54h20_initiating>`.

      To build and flash both images, run the following command to perform a :ref:`pristine build <zephyr:west-building>`:

      .. code-block:: console

         west build -p -b nrf54h20dk/nrf54h20/cpuppr
         west flash

   .. group-tab:: Using an application that supports multi-image builds

      If your application involves creating custom images for both the application core and the PPR core, disable the VPR bootstrapping sample by setting the :kconfig:option:`SB_CONFIG_VPR_LAUNCHER` option to ``n`` when building for the PPR target.
      For more details, see :ref:`how to configure Kconfig <configuring_kconfig>`.

      To build and flash both images, run the following command to perform a :ref:`pristine build <zephyr:west-building>`:

      .. code-block:: console

         west build -p -b nrf54h20dk/nrf54h20/cpuppr -- -DSB_CONFIG_VPR_LAUNCHER=n
         west flash

Separate images
---------------

You can build and program the application sample and the PPR sample as separate images using either |nRFVSC| or the command line.
Refer to `nRF Util`_ documentation for instructions on using nRF Util.
Depending on the method you select, complete the following steps:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      .. include:: /includes/vsc_build_and_run.txt

      3. Build the application image by configuring the following options:

         * Set the Board target to ``nrf54h20dk/nrf54h20/cpuapp``.
         * Select either the ``nordic-ppr`` or ``nordic-ppr-xip`` snippet, depending on the PPR image target.
         * Set System build to :guilabel:`No sysbuild`.

         For more information, see :ref:`cmake_options`.

      #. Build the PPR image by configuring the following options:

         * Set the Board target to ``nrf54h20dk/nrf54h20/cpuppr`` (recommended) or ``nrf54h20dk/nrf54h20/cpuppr/xip``.
         * Set System build to :guilabel:`No sysbuild`.

         For more information, see :ref:`cmake_options`.

   .. group-tab:: Command Line

      1. |open_terminal_window_with_environment|
      #. Build the application core image, and based on your build target, include the appropriate snippet:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuapp -S nordic-ppr --no-sysbuild

      #. Program the application core image by running the ``west flash`` command :ref:`without --erase <programming_params_no_erase>`.

         .. code-block:: console

            west flash

      #. Build the PPR core image:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuppr --no-sysbuild

         You can customize the command for additional options by adding :ref:`build parameters <optional_build_parameters>`.

      #. Once the PPR core image is successfully built, program it by running the ``west flash`` command :ref:`without --erase <programming_params_no_erase>`.

         .. code-block:: console

            west flash
