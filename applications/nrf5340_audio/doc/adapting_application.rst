.. _nrf53_audio_app_adapting:

Adapting nRF5340 Audio applications for end products
####################################################

.. contents::
   :local:
   :depth: 2

This page describes the relevant configuration sources and lists the steps required for adapting the :ref:`nrf53_audio_app` to end products.

Board configuration sources
***************************

The nRF5340 Audio applications use the following files as board configuration sources:

* Devicetree Specification (DTS) files - These reflect the hardware configuration.
  See :ref:`zephyr:dt-guide` for more information about the DTS data structure.
* Kconfig files - These reflect the hardware-related software configuration.
  See :ref:`kconfig_tips_and_tricks` for information about how to configure them.
* Memory layout configuration files - These define the memory layout of the application.

You can see the :file:`zephyr/boards/arm/nrf5340_audio_dk_nrf5340` directory as an example of how these files are structured.

For information about differences between DTS and Kconfig, see :ref:`zephyr:dt_vs_kconfig`.
For detailed instructions for adding Zephyr support to a custom board, see Zephyr's :ref:`zephyr:board_porting_guide`.

.. _nrf53_audio_app_porting_guide_app_configuration:

Application configuration sources
*********************************

The application configuration source file defines a set of options used by the given nRF5340 Audio application.
This is a :file:`.conf` file that modifies the default Kconfig values defined in the Kconfig files.

Only one :file:`.conf` file is included at a time.
The :file:`prj.conf` file is the default configuration file and it implements the debug application version.
For the release application version, you need to include the :file:`prj_release.conf` configuration file.
In the release application version no debug features should be enabled.

Each nRF5340 Audio application also uses its own :file:`Kconfig.default` file to change configuration defaults automatically.

You need to edit :file:`prj.conf` and :file:`prj_release.conf` if you want to add new functionalities to your application, but editing these files when adding a new board is not required.

.. _nrf53_audio_app_porting_guide_adding_board:

Adding a new board
******************

.. note::
    The first three steps of the configuration procedure are identical to the steps described in Zephyr's :ref:`zephyr:board_porting_guide`.

To use the nRF5340 Audio application with your custom board:

1. Define the board files for your custom board:

   a. Create a new directory in the :file:`nrf/boards/arm/` directory with the name of the new board.
   #. Copy the nRF5340 Audio board files from the :file:`nrf5340_audio_dk_nrf5340` directory located in the :file:`zephyr/boards/arm/` folder to the newly created directory.

#. Edit the DTS files to make sure they match the hardware configuration.
   Pay attention to the following elements:

   * Pins that are used.
   * Interrupt priority that might be different.

#. Edit the board's Kconfig files to make sure they match the required system configuration.
   For example, disable the drivers that will not be used by your device.
#. Build the application by selecting the name of the new board (for example, ``new_audio_board_name``) in your build system.
   For example, when building from the command line, add ``-b new_audio_board_name`` to your build command.

FOTA for end products
*********************

Do not use the default MCUboot key for end products.
See :ref:`ug_fw_update` and :ref:`west-sign` for more information.

To create your own app that supports DFU, you can use the `nRF Connect Device Manager`_ libraries for Android and iOS.

Changing default values
***********************

Given the requirements for the Coordinated Set Identification Service (CSIS), make sure to change the Set Identity Resolving Key (SIRK) value when adapting the application.
