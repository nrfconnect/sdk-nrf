.. _ug_nrf9160:

Working with nRF9160 DK
#######################

.. contents::
   :local:
   :depth: 2

See the `nRF9160 DK Hardware`_ guide for detailed information about the nRF9160 DK hardware.
To get started with the nRF9160 DK, follow the steps in the `nRF9160 DK Getting Started`_ guide.

.. _nrf9160_ug_intro:

Board controller
===============

The nRF9160 DK contains an nRF52840 SoC that is used to route some of the nRF9160 SiP pins to different components on the DK, such as LEDs and buttons, and to specific pins of the nRF52840 SoC itself.
For a complete list of all the routing options available, see the `nRF9160 DK board control section in the nRF9160 DK User Guide`_.

The nRF52840 SoC on the DK comes preprogrammed with a firmware.
If you need to restore the original firmware at some point, download the nRF9160 DK board controller FW from the `nRF9160 DK product page`_.
To program the HEX file, use nrfjprog (which is part of the `nRF Command Line Tools`_).

If you want to route some pins differently from what is done in the preprogrammed firmware, program the :ref:`zephyr:hello_world` sample instead of the preprogrammed firmware.
Build the sample (located under ``ncs/zephyr/samples/hello_world``) for the nrf9160dk_nrf52840 board.
To change the routing options, enable or disable the corresponding devicetree nodes for that board as needed.
See :ref:`zephyr:nrf9160dk_board_controller_firmware` for detailed information.

Board revisions
===============

nRF9160 DK v0.14.0 and later has additional hardware features that are not available on earlier versions of the DK:

* External flash memory
* I/O expander

To make use of these features, specify the board revision when building your application.

.. note::
   You must specify the board revision only if you use features that are not available in all board revisions.
   If you do not specify a board revision, the firmware is built for the default revision (v0.7.0).
   Newer revisions are compatible with the default revision.

To specify the board revision, append it to the build target when building.
For example, when building a non-secure application for nRF9160 DK v1.0.0, use ``nrf9160dk_nrf9106ns@1.0.0`` as build target.

When building with |SES|, specify the board revision as additional CMake option (see :ref:`cmake_options` for instructions).
For example, for nRF9160 DK v1.0.0, add the following CMake option::

  -DBOARD=nrf9160dk_nrf9160_ns@1.0.0

See :ref:`zephyr:application_board_version` and :ref:`zephyr:nrf9160dk_additional_hardware` for more information.


.. _nrf9160_ug_drivs_libs_samples:

Available drivers, libraries, and samples
*****************************************

See the :ref:`drivers`, :ref:`libraries`, and :ref:`nRF9160 samples <nrf9160_samples>` sections and the respective repository folders for up-to-date information.
