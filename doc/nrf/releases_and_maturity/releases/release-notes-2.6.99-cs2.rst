.. _ncs_release_notes_2699_cs2:

|NCS| v2.6.99-cs2 Release Notes
###############################

.. contents::
   :local:
   :depth: 3

|NCS| v2.6.99-cs2 is a **customer sampling** release, tailored exclusively for participants in the nRF54H20 SoC customer sampling program.
Do not use this release of |NCS| if you are not participating in the program.

The release is not part of the regular release cycle and is specifically designed to support early adopters working with the nRF54H20 SoC.
It is intended to provide early access to the latest developments for nRF54H20 SoC, enabling participants to experiment with new features and provide feedback.

The scope of this release is intentionally narrow, concentrating solely on the nRF54H20 DK.
You can use the latest stable release of |NCS| to work with other boards.

All functionality related to nRF54H20 SoC is considered experimental.

Migration notes
***************

See the :ref:`migration_cs3_to_2_6_99_cs2` for the changes required or recommended when migrating your application from v2.4.99-cs3 to v2.6.99-cs2.

Changelog
*********

The following sections provide detailed lists of changes by component.

Peripherals support
===================

* Added experimental support for the following peripherals on the nRF54H20 DK:

  * ADC
  * CLOCK
  * GPIO
  * GRTC
  * I2C
  * MRAM
  * NFCT
  * PWM
  * RADIO (Bluetooth Low Energy and 802.15.4)
  * SPI
  * UART
  * WDT

Working with nRF54H Series
==========================

* Added the :ref:`ug_nrf54h` section.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Support for the :ref:`nRF54H20 DK <ug_nrf54h20_gs>` board.
  * Fast switching between radio states for the nRF54H20 SoC.
  * Fast radio channel switching for the nRF54H20 SoC.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* :ref:`direct_test_mode` sample:

  * Added:

    * Support for the :ref:`nRF54H20 DK <ug_nrf54h20_gs>` with build target ``nrf54h20dk_nrf54h20_cpurad``.
    * Experimental support for the HCI interface.

  * Updated the internal sample API.

Enhanced ShockBurst samples
---------------------------

* Added support for the :ref:`nRF54H20 DK <ug_nrf54h20_gs>` with build target ``nrf54h20dk_nrf54h20_cpurad`` in the following ESB samples:

  * :ref:`esb_prx` sample
  * :ref:`esb_ptx` sample

Multicore samples
-----------------

* ``multicore_hello_world`` sample:

  * Added support for the :ref:`nRF54H20 DK <ug_nrf54h20_gs>` with build target ``nrf54h20dk_nrf54h20_cpuapp``.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added support for the :ref:`nRF54H20 DK <ug_nrf54h20_gs>` with build target ``nrf54h20dk_nrf54h20_cpurad``.
  * Updated the CLI command ``fem tx_power_control <tx_power_control>`` which replaces ``fem tx_gain <tx_gain>`` .
    This change applies to the sample built with the Kconfig option :ref:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC <CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC>` set to ``n``.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``0051731a41fa2c9057f360dc9b819e47b2484be5``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into the |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 0051731a41 ^23cf38934c

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^0051731a41

The current |NCS| main branch is based on revision ``0051731a41`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Zephyr samples
--------------

* Added support for the :ref:`nRF54H20 DK <ug_nrf54h20_gs>` in the following Zephyr samples:

  * :zephyr:code-sample:`pwm-blinky`
  * :zephyr:code-sample:`hello_world`
  * :zephyr:code-sample:`fade-led`

Documentation
=============

* Added:

  * The :ref:`ug_nrf54h` section.
  * The `Migration guide for nRF Connect SDK v2.6.99_cs2 for v2.4.99-cs3 users`_ section.

* Updated the table listing the :ref:`boards included in sdk-zephyr <app_boards_names_zephyr>` with the nRF54H20 DK board.

Current nRF54H20 DK Limitations
*******************************

* On the nRF54H20 DK Revision PCA10175 v0.7.x, the **ON** and **OFF** markings for the power switch on the PCB are switched.
* On all the revisions of the nRF54H20 DK, buttons and LEDs on the PCB are numbered from 0 to 3 instead of from 1 to 4.
