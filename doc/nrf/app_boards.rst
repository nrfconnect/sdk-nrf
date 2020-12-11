.. _app_boards:

Board support
#############

.. contents::
   :local:
   :depth: 2

The |NCS| provides board definitions for all Nordic Semiconductor devices.
In addition, you can define custom boards.

.. _gs_programming_board_names:

Board names
***********

The following tables list all boards and build targets for Nordic Semiconductor's hardware platforms.

Boards included in sdk-zephyr
=============================

The following boards are defined in the :file:`zephyr/boards/arm/` folder.
Also see the :ref:`zephyr:boards` section in the Zephyr documentation.

.. _table:

+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| Hardware platform | PCA number | Board name                                                      | Build target                          |
+===================+============+=================================================================+=======================================+
| nRF52 DK          | PCA10040   | :ref:`nrf52dk_nrf52832 <zephyr:nrf52dk_nrf52832>`               | ``nrf52dk_nrf52832``                  |
| (nRF52832)        |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52dk_nrf52810 <zephyr:nrf52dk_nrf52810>`               | ``nrf52dk_nrf52810``                  |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF52833 DK       | PCA10100   | :ref:`nrf52833dk_nrf52833 <zephyr:nrf52833dk_nrf52833>`         | ``nrf52833dk_nrf52833``               |
|                   |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52833dk_nrf52820 <zephyr:nrf52833dk_nrf52820>`         | ``nrf52833dk_nrf52820``               |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF52840 DK       | PCA10056   | :ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`         | ``nrf52840dk_nrf52840``               |
|                   |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52840dk_nrf52811 <zephyr:nrf52840dk_nrf52811>`         | ``nrf52840dk_nrf52811``               |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF52840 Dongle   | PCA10059   | :ref:`nrf52840dongle_nrf52840 <zephyr:nrf52840dongle_nrf52840>` | ``nrf52840dongle_nrf52840``           |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| Thingy:52         | PCA20020   | :ref:`thingy52_nrf52832 <zephyr:thingy52_nrf52832>`             | ``thingy52_nrf52832``                 |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF21540 DK       | PCA10112   | :ref:`nrf21540dk_nrf52840 <zephyr:nrf21540dk_nrf52840>`         | ``nrf21540dk_nrf52840``               |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF5340 DK        | PCA10095   | :ref:`nrf5340dk_nrf5340 <zephyr:nrf5340dk_nrf5340>`             | ``nrf5340dk_nrf5340_cpunet``          |
|                   |            |                                                                 |                                       |
|                   |            |                                                                 | ``nrf5340dk_nrf5340_cpuapp``          |
|                   |            |                                                                 |                                       |
|                   |            |                                                                 | ``nrf5340dk_nrf5340_cpuappns``        |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF9160 DK        | PCA10090   | :ref:`nrf9160dk_nrf9160 <zephyr:nrf9160dk_nrf9160>`             | ``nrf9160dk_nrf9160``                 |
|                   |            |                                                                 |                                       |
|                   |            |                                                                 | ``nrf9160dk_nrf9160ns``               |
|                   |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf9160dk_nrf52840 <zephyr:nrf9160dk_nrf52840>`           | ``nrf9160dk_nrf52840``                |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+


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
|                   |            |                                                          | ``thingy91_nrf9160ns``                |
|                   |            +----------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`thingy91_nrf52840 <ug_thingy91>`                   | ``thingy91_nrf52840``                 |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+


Custom boards
*************

Defining your own board is a very common step in application development, since applications are typically designed to run on boards that are not directly supported by the |NCS|, given that they are typically custom designs and not available publicly.
To define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.
