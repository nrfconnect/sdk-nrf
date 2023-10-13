.. _app_boards:

Board support
#############

.. contents::
   :local:
   :depth: 2

The |NCS| provides board definitions for all Nordic Semiconductor devices.
In addition, you can define custom boards.

.. _app_boards_names:
.. _programming_board_names:

Board names
***********

The following tables list all boards and build targets for Nordic Semiconductor's hardware platforms.

The build target column uses several entries for multi-core hardware platforms:

* For core type:

  * ``cpuapp`` - When you choose this target, you build the application core firmware.
  * ``cpunet`` - When you choose this target, you build the network core firmware.

* For usage of Cortex-M Security Extensions (CMSE):

  * Entries without ``*_ns`` (``cpuapp``) - When you choose this target, you build the application core firmware as a single execution environment that does not use CMSE (:ref:`Trusted Firmware-M (TF-M) <ug_tfm>`).
  * Entries with ``*_ns`` (for example, ``cpuapp_ns``) - When you choose this target, you build the application with CMSE.
    The application core firmware is placed in Non-Secure Processing Environment (NSPE) and uses Secure Processing Environment (SPE) for security features.
    By default, the build system automatically includes :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` in SPE and merges it with NSPE.

Read more about separation of processing environments in the :ref:`app_boards_spe_nspe` section.

.. _app_boards_names_zephyr:

Boards included in sdk-zephyr
=============================

The following boards are defined in the :file:`zephyr/boards/arm/` folder.
Also see the :ref:`zephyr:boards` section in the Zephyr documentation.

.. _table:

+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| Hardware platform | PCA number | Board name                                                        | Build target                          |
+===================+============+===================================================================+=======================================+
| nRF52 DK          | PCA10040   | :ref:`nrf52dk_nrf52832 <zephyr:nrf52dk_nrf52832>`                 | ``nrf52dk_nrf52832``                  |
| (nRF52832)        |            +-------------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52dk_nrf52810 <zephyr:nrf52dk_nrf52810>`                 | ``nrf52dk_nrf52810``                  |
|                   |            +-------------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52dk_nrf52805 <zephyr:nrf52dk_nrf52805>`                 | ``nrf52dk_nrf52805``                  |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF52833 DK       | PCA10100   | :ref:`nrf52833dk_nrf52833 <zephyr:nrf52833dk_nrf52833>`           | ``nrf52833dk_nrf52833``               |
|                   |            +-------------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52833dk_nrf52820 <zephyr:nrf52833dk_nrf52820>`           | ``nrf52833dk_nrf52820``               |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF52840 DK       | PCA10056   | :ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`           | ``nrf52840dk_nrf52840``               |
|                   |            +-------------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52840dk_nrf52811 <zephyr:nrf52840dk_nrf52811>`           | ``nrf52840dk_nrf52811``               |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF52840 Dongle   | PCA10059   | :ref:`nrf52840dongle_nrf52840 <zephyr:nrf52840dongle_nrf52840>`   | ``nrf52840dongle_nrf52840``           |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| Thingy:52         | PCA20020   | :ref:`thingy52_nrf52832 <zephyr:thingy52_nrf52832>`               | ``thingy52_nrf52832``                 |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF21540 DK       | PCA10112   | :ref:`nrf21540dk_nrf52840 <zephyr:nrf21540dk_nrf52840>`           | ``nrf21540dk_nrf52840``               |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF5340 DK        | PCA10095   | :ref:`nrf5340dk_nrf5340 <zephyr:nrf5340dk_nrf5340>`               | ``nrf5340dk_nrf5340_cpunet``          |
|                   |            |                                                                   |                                       |
|                   |            |                                                                   | ``nrf5340dk_nrf5340_cpuapp``          |
|                   |            |                                                                   |                                       |
|                   |            |                                                                   | ``nrf5340dk_nrf5340_cpuapp_ns``       |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| Thingy:53         | PCA20053   | :ref:`thingy53_nrf5340 <zephyr:thingy53_nrf5340>`                 | ``thingy53_nrf5340_cpunet``           |
|                   |            |                                                                   |                                       |
|                   |            |                                                                   | ``thingy53_nrf5340_cpuapp``           |
|                   |            |                                                                   |                                       |
|                   |            |                                                                   | ``thingy53_nrf5340_cpuapp_ns``        |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF5340 Audio     | PCA10121   | :ref:`nrf5340_audio_dk_nrf5340 <zephyr:nrf5340_audio_dk_nrf5340>` |  ``nrf5340_audio_dk_nrf5340_cpuapp``  |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF9160 DK        | PCA10090   | :ref:`nrf9160dk_nrf9160 <zephyr:nrf9160dk_nrf9160>`               | ``nrf9160dk_nrf9160``                 |
|                   |            |                                                                   |                                       |
|                   |            |                                                                   | ``nrf9160dk_nrf9160_ns``              |
|                   |            +-------------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf9160dk_nrf52840 <zephyr:nrf9160dk_nrf52840>`             | ``nrf9160dk_nrf52840``                |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+
| nRF9161 DK        | PCA10153   | :ref:`nrf9161dk_nrf9161 <zephyr:nrf9161dk_nrf9161>`               | ``nrf9161dk_nrf9161``                 |
|                   |            |                                                                   |                                       |
|                   |            |                                                                   | ``nrf9161dk_nrf9161_ns``              |
+-------------------+------------+-------------------------------------------------------------------+---------------------------------------+

.. note::
   In |NCS| releases before v1.6.1:

   * The build target ``nrf9160dk_nrf9160_ns`` was named ``nrf9160dk_nrf9160ns``.
   * The build target ``nrf5340dk_nrf5340_cpuapp_ns`` was named ``nrf5340dk_nrf5340_cpuappns``.

.. _app_boards_names_nrf:

Boards included in sdk-nrf
==========================

The following boards are defined in the :file:`nrf/boards/arm/` folder.

+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| Hardware platform | PCA number | Board name                                               | Build target                          |
+===================+============+==========================================================+=======================================+
| nRF Desktop       | PCA20041   | :ref:`nrf52840gmouse_nrf52840 <nrf_desktop>`             | ``nrf52840gmouse_nrf52840``           |
| Gaming Mouse      |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA20044   | :ref:`nrf52dmouse_nrf52832 <nrf_desktop>`                | ``nrf52dmouse_nrf52832``              |
| Mouse             |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA20045   | :ref:`nrf52810dmouse_nrf52810 <nrf_desktop>`             | ``nrf52810dmouse_nrf52810``           |
| Mouse             |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA20037   | :ref:`nrf52kbd_nrf52832 <nrf_desktop>`                   | ``nrf52kbd_nrf52832``                 |
| Keyboard          |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA10111   | :ref:`nrf52833dongle_nrf52833 <nrf_desktop>`             | ``nrf52833dongle_nrf52833``           |
| Dongle            |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA10114   | :ref:`nrf52820dongle_nrf52820 <nrf_desktop>`             | ``nrf52820dongle_nrf52820``           |
| Dongle            |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| Thingy:91         | PCA20035   | :ref:`thingy91_nrf9160 <ug_thingy91>`                    | ``thingy91_nrf9160``                  |
|                   |            |                                                          |                                       |
|                   |            |                                                          | ``thingy91_nrf9160_ns``               |
|                   |            +----------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`thingy91_nrf52840 <ug_thingy91>`                   | ``thingy91_nrf52840``                 |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF7002 DK        | PCA10143   | :ref:`nrf7002dk_nrf5340 <nrf7002dk_nrf5340>`             | ``nrf7002dk_nrf5340_cpunet``          |
|                   |            |                                                          |                                       |
|                   |            |                                                          | ``nrf7002dk_nrf5340_cpuapp``          |
|                   |            |                                                          |                                       |
|                   |            |                                                          | ``nrf7002dk_nrf5340_cpuapp_ns``       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+

The :ref:`nRF21540 EK shield <ug_radio_fem_nrf21540ek>` is defined in the :file:`nrf/boards/shields` folder.

Custom boards
*************

Defining your own board is a very common step in application development, because applications are typically designed to run on boards that are not directly supported by the |NCS|, and are often custom designs not available publicly.
To define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

One of the |NCS| applications that lets you add custom boards is :ref:`nrf_desktop`.
See :ref:`nrf_desktop_porting_guide` in the application documentation for details.

.. _app_boards_spe_nspe:

Processing environments
***********************

The build target column in the tables above separates entries according to the CPU to target (for multi-core SoCs) and whether Cortex-M Security Extensions (CMSE) are used or not (addition of ``_ns`` if they are used).

When CMSE is used, the firmware is split in accordance with the security by separation architecture principle to better protect sensitive assets and code.
With CMSE, the firmware is stored in one of two security environments (flash partitions), either Secure Processing Environment (SPE) or Non-Secure Processing Environment (NSPE).
This isolation of firmware is only possible if the underlying hardware supports `ARM TrustZone`_.

.. figure:: images/spe_nspe.svg
   :alt: Processing environments in the |NCS|

   Processing environments in the |NCS|

In Zephyr and the |NCS|, SPE and NSPE are used exclusively in the context of the application core of a multi-core SoC.
Building follows the security by separation principle and depends on the build target.

.. _app_boards_spe_nspe_cpuapp:

Building for ``cpuapp`` (CMSE disabled)
=======================================

When you build for ``cpuapp``, you build the firmware for the application core without CMSE.
Because CMSE is disabled, TF-M is not used and there is no separation of firmware.

.. _app_boards_spe_nspe_cpuapp_ns:

Building for ``*_ns`` (CMSE enabled)
====================================

When you build for ``*_ns``, you build firmware with CMSE.
Firmware is separated in the following way:

* SPE implements security-critical functionality and data (including bootloaders) and isolates them from the application software in NSPE.
  It also contains secure firmware running in the secure state.
* NSPE typically implements the user application and communication firmware, among other major components.

The application is built as a non-secure image and :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` is built as the secure image.
The build system merges both images to form a combined image that will be used for programming or updating the device.

TF-M enables hardware-supported separation of firmware.
It also implements `Platform Security Architecture (PSA)`_ API, which provides security features for the system, including roots of trust for protecting secrets, platform state, and cryptographic keys.
The API coordinates the communication with the components in NSPE.

More information about SPE and NSPE
-----------------------------------

Read the following pages for a better understanding of security by separation in the |NCS|:

* :ref:`ug_tfm`
* :ref:`debugging_spe_nspe`
* `An Introduction to Trusted Firmware-M (TF-M)`_ on DevZone
* `TF-M documentation`_
