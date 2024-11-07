.. _building_nrf54l:

Building and programming with nRF54L15 DK
#########################################

.. contents::
   :local:
   :depth: 2

.. note::
   The FLPR core support in the |NCS| is currently :ref:`experimental<software_maturity>`.

This guide provides instructions on how to build and program the nRF54L15 development kit.
Whether you are working with single or multi-image builds, the following sections will guide you through the necessary steps.

Depending on the sample, you must program only the application core or both the Fast Lightweight Peripheral Processor (FLPR) and the application core.
Additionally, the process will differ based on whether you are working with a single-image or multi-image build.

.. note::
   The following instructions do not include multi-image single-core builds scenario.

Building for the application core only
**************************************

Building for the application core only follows the default building process for the |NCS|.
For instructions, see how the :ref:`building` page.

.. _building_nrf54l_app_flpr_core:

Building for the application and FLPR core
******************************************

Building for both the application and the FLPR cores is different from the default |NCS| procedure.
Using the FLPR core also requires additional configuration to enable it.
This section outlines how to build and program for both the application and FLPR core, covering separate builds and sysbuild configurations.
The FLPR core supports two variants:

* ``nrf54l15dk/nrf54l15/cpuflpr``, where FLPR runs from SRAM, which is the recommended method.
  To build FLPR image with this variant, the application core image must include the ``nordic-flpr`` :ref:`snippet <app_build_snippets>`.

* ``nrf54l15dk/nrf54l15/cpuflpr/xip``, where FLPR runs from RRAM.
  To build FLPR image with this variant, the application core image must include the ``nordic-flpr-xip`` snippet.

Standard build
--------------

This subsection focuses on how to build an application using :ref:`sysbuild <configuration_system_overview_sysbuild>`.

.. note::
   Currently, the documentation does not cover specific instructions for building an application image that uses sysbuild to incorporate the FLPR core as a sub-image.
   The only documented scenario is for building FLPR as the main image and the application as a sub-image.

Complete the following steps:

.. tabs::

   .. group-tab:: Using minimal sample for VPR bootstrapping

      This option automatically programs the FLPR core with :ref:`dedicated bootstrapping firmware <vpr_flpr_nrf54l15_initiating>`.

      To build and flash both images, run the following command that performs a :ref:`pristine build <zephyr:west-building>`:

      .. code-block:: console

         west build -p -b nrf54l15dk/nrf54l15/cpuflpr
         west flash

   .. group-tab:: Using application that supports multi-image builds

      If your application involves creating custom images for both the application core and the FLPR core, make sure to disable the VPR bootstrapping sample.
      You can do this by disabling the ``SB_CONFIG_VPR_LAUNCHER`` option when building for the FLPR target.
      For more details, see :ref:`how to configure Kconfig <configuring_kconfig>`.

      To build and flash both images, run the following command that performs a :ref:`pristine build <zephyr:west-building>`:

      .. code-block:: console

         west build -p -b nrf54l15dk/nrf54l15/cpuflpr -- -DSB_CONFIG_VPR_LAUNCHER=n
         west flash

Separate images
---------------

You can build and program application sample and the FLPR sample as separate images using the |nRFVSC| or command line.
To use nRF Util, see `Programming application firmware on the nRF54L15 SoC`_.
Depending on the selected method, complete the following steps:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      .. note::

         The |nRFVSC| currently offers experimental support for the nRF54L15's FLPR core.
         Certain features, particularly debugging, may not function as expected.

      .. include:: /includes/vsc_build_and_run.txt

      3. Build the application image by setting the following options:

         * Board target to ``nrf54l15dk/nrf54l15/cpuapp``.
         * Choose either ``nordic-flpr`` or ``nordic-flpr-xip`` snippet depending on the FLPR image target.
         * System build to :guilabel:`No sysbuild`.

         For more information, see :ref:`cmake_options`.

      #. Build the FLPR image by setting the following options:

         * Board target to ``nrf54l15dk/nrf54l15/cpuflpr`` (recommended) or ``nrf54l15dk/nrf54l15/cpuflpr/xip``.
         * System build to :guilabel:`No sysbuild`.

         For more information, see :ref:`cmake_options`.

   .. group-tab:: Command line

      1. |open_terminal_window_with_environment|
      #. Build the application core image, and based on your build target include the appropriate snippet:

         .. code-block:: console

            west build -p -b nrf54l15dk/nrf54l15/cpuapp -S nordic-flpr --no-sysbuild

      #. Program the application core image by running the `west flash` command :ref:`without --erase <programming_params_no_erase>`.

         .. code-block:: console

            west flash

      #. Build the FLPR core image:

         .. code-block:: console

            west build -p -b nrf54l15dk/nrf54l15/cpuflpr --no-sysbuild

         You can also customize the command for additional options, by adding :ref:`build parameters <optional_build_parameters>`.

      #. Once you have successfully built the FLPR core image, program it by running the `west flash` command :ref:`without --erase <programming_params_no_erase>`.

         .. code-block:: console

            west flash
