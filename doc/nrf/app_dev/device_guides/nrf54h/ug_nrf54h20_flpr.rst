.. _ug_nrf54h20_flpr:

Working with the FLPR core
##########################

.. contents::
   :local:
   :depth: 2

.. note::
   The FLPR core support in the |NCS| is currently :ref:`experimental<software_maturity>`.

The nRF54H20 SoC includes a dedicated VPR CPU, based on RISC-V architecture, known as the *fast lightweight peripheral processor* (FLPR).
The FLPR core can be used to manage specific peripherals through the appropriate Zephyr Device Driver API.
These peripherals have IRQs routed to FLPR:

* USBHS
* EXMIF
* I3C120
* CAN120
* I3C121
* TIMER120
* TIMER121
* PWM120
* SPIS120
* SPIM120/UARTE120
* SPIM121

All other peripherals available to the application core can also be used with FLPR.
However, they require the use of *polling mode*.

.. _vpr_flpr_nrf54h20_initiating:

Using Zephyr multithreaded mode on FLPR
***************************************

The FLPR core can operate as a general-purpose core, running under the full Zephyr kernel.
Building the FLPR target is similar to building the application core, but the application core build must include an overlay that enables the FLPR core.

Bootstrapping the FLPR core
===========================

The |NCS| provides a FLPR snippet that adds the overlay needed for bootstrapping the FLPR core.
The primary purpose of this snippet is to enable the transfer of the FLPR code to the designated memory region (if required) and initiate the FLPR core.

When building for the ``nrf54h20dk/nrf54h20/cpuflpr`` target, a minimal sample is automatically loaded onto the application core.
For more details, see :ref:`building_nrf54h_app_flpr_core`.

Memory allocation
*****************

Running the FLPR CPU can lead to increased latency when accessing ``RAM_21``.
To mitigate this, you should use ``RAM_21`` exclusively for FLPR code, FLPR data, and non-time-sensitive information from the application CPU.
For data that requires strict access times, such as CPU data used in low-latency ISRs, you should use local RAM or, when greater latency is acceptable, ``RAM_0x``.
The DMA buffers should be placed in memory designed to a given peripheral.

.. _building_nrf54h:

Building and programming with the nRF54H20 DK
*********************************************

.. note::
   The FLPR core support in the |NCS| is currently :ref:`experimental<software_maturity>`.

Depending on the sample, you may need to program only the application core or both the FLPR and application cores.
Additionally, the process will vary depending on whether you are working with a single-image or multi-image build.

.. note::
   The following instructions do not cover the scenario of multi-image single-core builds.

Building for the application core only
======================================

Building for the application core follows the default building process for the |NCS|.
For detailed instructions, refer to the :ref:`building` page.

.. _building_nrf54h_app_flpr_core:

Building for both the application and FLPR core
===============================================

Building for both the application core and the FLPR core differs from the default |NCS| procedure.
Additional configuration is required to enable the FLPR core.

This section explains how to build and program both cores, covering separate builds and sysbuild configurations.
The FLPR core supports two variants:

* ``nrf54h20dk/nrf54h20/cpuflpr``: FLPR runs from RAM_21 (recommended method).
  The application core image must include the ``nordic-flpr`` :ref:`snippet <app_build_snippets>`.

* ``nrf54h20dk/nrf54h20/cpuflpr/xip``: FLPR runs from MRAM.
  The application core image must include the ``nordic-flpr-xip`` snippet.

Standard build
--------------

This subsection explains how to build an application using :ref:`sysbuild <configuration_system_overview_sysbuild>`.

.. note::
   Currently, the documentation does not provide specific instructions for building an application image using sysbuild to incorporate the FLPR core as a sub-image.
   The only documented scenario involves building the FLPR as the main image and the application as a sub-image.

Follow these steps to complete the build:

.. tabs::

   .. group-tab:: Using minimal sample for VPR bootstrapping

      This option automatically programs the FLPR core with :ref:`dedicated bootstrapping firmware <vpr_flpr_nrf54h20_initiating>`.

      To build and flash both images, run the following command to perform a :ref:`pristine build <zephyr:west-building>`:

      .. code-block:: console

         west build -p -b nrf54h20dk/nrf54h20/cpuflpr
         west flash

   .. group-tab:: Using an application that supports multi-image builds

      If your application involves creating custom images for both the application core and the FLPR core, disable the VPR bootstrapping sample by setting the ``SB_CONFIG_VPR_LAUNCHER`` option to ``n`` when building for the FLPR target.
      For more details, see :ref:`how to configure Kconfig <configuring_kconfig>`.

      To build and flash both images, run the following command to perform a :ref:`pristine build <zephyr:west-building>`:

      .. code-block:: console

         west build -p -b nrf54h20dk/nrf54h20/cpuflpr -- -DSB_CONFIG_VPR_LAUNCHER=n
         west flash

Separate images
---------------

You can build and program the application sample and the FLPR sample as separate images using either the |nRFVSC| or the command line.
To use nRF Util, see `nRF Util`_.
Depending on the method you select, complete the following steps:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      .. note::

         The |nRFVSC| currently offers experimental support for the nrf54h20's FLPR core.
         Certain features, particularly debugging, may not function as expected.

      .. include:: /includes/vsc_build_and_run.txt

      3. Build the application image by configuring the following options:

         * Set the Board target to ``nrf54h20dk/nrf54h20/cpuapp``.
         * Select either the ``nordic-flpr`` or ``nordic-flpr-xip`` snippet, depending on the FLPR image target.
         * Set System build to :guilabel:`No sysbuild`.

         For more information, see :ref:`cmake_options`.

      #. Build the FLPR image by configuring the following options:

         * Set the Board target to ``nrf54h20dk/nrf54h20/cpuflpr`` (recommended) or ``nrf54h20dk/nrf54h20/cpuflpr/xip``.
         * Set System build to :guilabel:`No sysbuild`.

         For more information, see :ref:`cmake_options`.

   .. group-tab:: Command Line

      1. |open_terminal_window_with_environment|
      #. Build the application core image, and based on your build target, include the appropriate snippet:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuapp -S nordic-flpr --no-sysbuild

      #. Program the application core image by running the `west flash` command :ref:`without --erase <programming_params_no_erase>`.

         .. code-block:: console

            west flash

      #. Build the FLPR core image:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuflpr --no-sysbuild

         You can customize the command for additional options by adding :ref:`build parameters <optional_build_parameters>`.

      #. Once the FLPR core image is successfully built, program it by running the `west flash` command :ref:`without --erase <programming_params_no_erase>`.

         .. code-block:: console

            west flash
