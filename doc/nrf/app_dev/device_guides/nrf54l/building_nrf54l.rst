.. _building_nrf54l:

Building and programming with nRF54L15 DK
#########################################

This guide provides instructions on how to build and program the nRF54L15 development kit.
Whether you are working with single or multi-image builds, the following sections will guide you through the necessary steps.

Depending on the sample, you must program only the application core or both the FLPR and the application core.
Additionally, the process will differ based on whether you are working with a single-image or multi-image build.

Building for the application core
*********************************

For instructions on building for the application core only, see how to :ref:`program an application <programming_cmd>`.

Building for the application and FLPR core
******************************************

This section outlines how to build and program for both the application and FLPR cores, covering separate builds and sysbuild configurations.

Separate images
---------------

To build and program the application sample and the FLPR sample as separate images, follow these steps:

.. tabs::

   .. group-tab:: west

      1. |open_terminal_window_with_environment|
      #. For the FLPR core, erase the flash memory and program the FLPR sample:

         .. Commands TBD

         .. code-block:: console

            west flash --erase

      #. For the application core, navigate to the build folder of the application sample and repeat the flash erase command:

         .. code-block:: console

            west flash --erase

   .. group-tab:: nRF Util

      1. |open_terminal_window_with_environment|
      #. For the FLPR core, run the following command to erase the flash memory of the FLPR core and program the sample:

         .. Commands TBD

         .. code-block:: console

            nrfutil device program --firmware zephyr.hex --options chip_erase_mode=ERASE_ALL --core Network

      #. For the application core, navigate to its build folder and run:

         .. code-block:: console

            nrfutil device program --firmware zephyr.hex  --options chip_erase_mode=ERASE_ALL

         .. note::
            The application build folder will be in a sub-directory which is the name of the folder of the application

      #. Reset the development kit:

         .. Commands TBD

         .. code-block:: console

            nrfutil device reset --reset-kind=RESET_PIN

         See :ref:`readback_protection_error` if you encounter an error.

With sysbuild
-------------

To build and program with sysbuild, complete the following steps:

.. tabs::

   .. group-tab:: Using VPR Launcher

      Instructions TBA

      .. code-block:: console

         # Example command TBD

   .. group-tab:: Using application that aupports multi-image builds

      Instructions TBA

      .. code-block:: console

         # Example command TBD
