.. _ug_nrf7002eb2_gs:

Developing with nRF7002-EB II
#############################

.. contents::
   :local:
   :depth: 2

The nRF7002 :term:`Expansion Board (EB)` II (PCA63571), part of the `nRF70 Series Family <nRF70 Series product page_>`_, can be used to provide Wi-Fi® connectivity to compatible development or evaluation boards through the nRF7002 Wi-Fi 6 companion IC.

You can use the nRF7002-EB II to provide Wi-Fi connectivity to the :zephyr:board:`nrf54l15dk` and :zephyr:board:`nrf54lm20dk` board targets.

.. figure:: images/nRF7002eb2.png
   :alt: nRF7002-EB II

   nRF7002-EB II

Pin mapping for the nRF54L15 DK and the nRF54LM20 DK
****************************************************

For the nRF54L15 DK and nRF54LM20 DK, refer to the following tables for the pin mappings for these kits.
The board can be mounted on the **P1** connector of the nRF54L15 DK and the **P17** (Expansion) connector of the nRF54LM20 DK.

.. tabs::

   .. group-tab:: nRF54L15 DK

      +-----------------------------------+-------------------+-----------------------------------------------+
      | nRF70 Series pin name (EB name)   | nRF54L15 DK pins  | Function                                      |
      +===================================+===================+===============================================+
      | CLK (CLK)                         | P1.11             | SPI Clock                                     |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | CS (CS)                           | P1.10             | SPI Chip Select                               |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | MOSI (D0)                         | P1.06             | SPI MOSI                                      |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | MISO (D1)                         | P1.07             | SPI MISO                                      |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | BUCKEN (EN)                       | P1.04             | Enable Buck regulator                         |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | IOVDDEN                           | P1.05             | Enable IOVDD regulator                        |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | IRQ (IRQ)                         | P1.14             | Interrupt from nRF7002                        |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | GRANT (GRT)                       | P1.12             | Coexistence grant from nRF7002                |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | REQ (REQ)                         | P1.09             | Coexistence request to nRF7002                |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | STATUS (ST0)                      | P1.08             | Coexistence status from nRF7002               |
      +-----------------------------------+-------------------+-----------------------------------------------+

   .. group-tab:: nRF54LM20 DK

      +-----------------------------------+-------------------+-----------------------------------------------+
      | nRF70 Series pin name (EB name)   | nRF54LM20 DK pins | Function                                      |
      +===================================+===================+===============================================+
      | CLK (CLK)                         | P3.03             | SPI Clock                                     |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | CS (CS)                           | P3.02             | SPI Chip Select                               |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | MOSI (D0)                         | P3.00             | SPI MOSI                                      |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | MISO (D1)                         | P3.01             | SPI MISO                                      |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | BUCKEN (EN)                       | P1.04             | Enable Buck regulator                         |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | IOVDDEN                           | P1.13             | Enable IOVDD regulator                        |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | IRQ (IRQ)                         | P1.05             | Interrupt from nRF7002                        |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | GRANT (GRT)                       | P1.07             | Coexistence grant from nRF7002                |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | REQ (REQ)                         | P0.04             | Coexistence request to nRF7002                |
      +-----------------------------------+-------------------+-----------------------------------------------+
      | STATUS (ST0)                      | P0.03             | Coexistence status from nRF7002               |
      +-----------------------------------+-------------------+-----------------------------------------------+

.. _nrf7002eb2_building_programming:

Building and programming with nRF7002-EB II
*******************************************

To build an application with support for Wi-Fi using the nRF7002-EB II, use a compatible :ref:`board target <app_boards_names>` with the CMake ``SHIELD`` option set to the corresponding shield name.
See :ref:`cmake_options` for instructions on how to provide CMake options.

For example, if you build the :ref:`wifi_shell_sample` sample for the nRF54L15 DK from the command line, use the following command:

.. code-block:: console

   west build -p -b nrf54l15dk/nrf54l15/cpuapp -- -Dshell_SHIELD="nrf7002eb2" -Dshell_SNIPPET=nrf70-wifi

If you use |nRFVSC|, specify ``-DSHIELD=nrf7002eb2`` in the **Extra Cmake arguments** field when `setting up a build configuration <How to work with build configurations_>`_.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file by using the following command:

.. code-block:: console

   set(SHIELD nrf7002eb2)

To build an application with support for Wi-Fi using the nRF7002-EB II with the nRF54LM20 DK, use the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the CMake ``SHIELD`` variable set to ``nrf7002eb2``.
To build an application with support for Wi-Fi for a custom target, set ``-DSHIELD="nrf7002eb2"`` when you invoke ``west build`` or ``cmake`` in your |NCS| application.

Alternatively, you can add the shield in the project's :file:`CMakeLists.txt` file by using the ``set(SHIELD nrf7002eb2)`` command.

To build an application with support for Wi-Fi using the nRF7002-EB II with the nRF54L15 DK, use the ``nrf54l15dk/nrf54l15/cpuapp`` board target with the CMake ``SHIELD`` variable set to ``nrf7002eb2``.
To build for a custom target, set ``-DSHIELD="nrf7002eb2"`` when you invoke ``west build`` or ``cmake`` in your |NCS| application.

Alternatively, you can add the shield in the project's :file:`CMakeLists.txt` file by using the ``set(SHIELD  nrf7002eb2)`` command.

To build with coexistence mode enabled, set ``-DSHIELD="nrf7002eb2;nrf7002eb2_coex"`` when you invoke ``west build`` or ``cmake`` in your |NCS| application.
For optimal build configuration, use the ``nrf70-wifi`` snippet.

Limitations when building with nRF54L15 DK and nRF54LM20 DK
***********************************************************

The Wi-Fi support is experimental and has the following limitations:

* It is suitable only for low-throughput applications.
* The Wi-Fi performance is not optimized.
  Refer to the :ref:`wifi_samples` documentation for the supported samples.
